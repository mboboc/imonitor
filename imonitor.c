/*
 * iMonitor via webserver
 * 
 * Source file
 */

#include "imonitor.h"
#include "debug.h"
#include "http-parser/http_parser.h"
#include "sock_util.h"
#include "util.h"
#include "w_epoll.h"

/* Server socket file descriptor */
static int listenfd;

/* epoll file descriptor */
static int epollfd;

/* Inotify file descriptor */
static int inotifyfd;

/* WTMP file descriptor */
static int wtmpfd;

/* Logfile file descriptor */
static int logfd;

enum connection_state {
	STATE_DATA_RECEIVED,
	STATE_CONNECTION_CLOSED,
	STATE_READING,
	STATE_SENDING,
	STATE_NEW_CONN
};

/* Structure acting as a connection handler */
struct connection {
	int sockfd;
	char recv_buffer[BUFSIZ];
	size_t recv_len;
	char send_buffer[BUFSIZ];
	size_t send_len;
	enum connection_state state;
	char request_path[BUFSIZ];
	int eventfd;
	size_t file_dimension;
	io_context_t ctx;
	size_t header_size;
	size_t data_size;
	size_t is_header;
	size_t read_data;
	int fd;
};

static http_parser request_parser;
static char request_path[BUFSIZ];

/*
 * Callback is invoked by HTTP request parser when parsing request path
 * Request path is stored in global request_path variable
 */
static int on_path_cb(http_parser *p, const char *buf, size_t len)
{
	assert(p == &request_parser);
	memcpy(request_path, buf, len);

	return 0;
}

/* Use mostly null settings except for on_path callback */
static http_parser_settings settings_on_path = {
	/* on_message_begin */ 0,
	/* on_header_field */ 0,
	/* on_header_value */ 0,
	/* on_path */ on_path_cb,
	/* on_url */ 0,
	/* on_fragment */ 0,
	/* on_query_string */ 0,
	/* on_body */ 0,
	/* on_headers_complete */ 0,
	/* on_message_complete */ 0
};

/*
 * Initialize connection structure on given socket
 */
static struct connection *connection_create(int sockfd)
{
	struct connection *conn;

	conn = malloc(sizeof(*conn));
	DIE(conn == NULL, "malloc");

	conn->sockfd = sockfd;
	memset(conn->recv_buffer, 0, BUFSIZ);
	memset(conn->send_buffer, 0, BUFSIZ);

	return conn;
}

/*
 * Remove connection handler
 */
static void connection_remove(struct connection *conn)
{
	close(conn->fd);
	io_destroy(conn->ctx);
	close(conn->sockfd);
	conn->state = STATE_CONNECTION_CLOSED;
	free(conn);
}

/*
 * Handle a new connection request on the server socket
 */
static void handle_new_connection(void)
{
	static int sockfd;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	struct connection *conn;
	int rc, flags;

	/* Accept new connection */
	sockfd = accept(listenfd, (SSA *)&addr, &addrlen);
	DIE(sockfd < 0, "accept");

	/* Set comm socket as non-blocking */
	flags = fcntl(sockfd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(sockfd, F_SETFL, flags);

	dlog(LOG_INFO, "Accepted connection from: %s:%d\n",
	     inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	/* Instantiate new connection handler */
	conn = connection_create(sockfd);
	conn->send_len = 0;
	conn->is_header = FALSE;
	conn->state = STATE_NEW_CONN;
	conn->ctx = 0;

	/* Setup aio context */
	rc = io_setup(1, &(conn->ctx));
	DIE(rc < 0, "io_setup failed");

	/* Add socket to epoll */
	rc = w_epoll_add_ptr_in(epollfd, sockfd, conn);
	DIE(rc < 0, "w_epoll_add_in");
}

/*
 * Receive message on socket, store it in recv_buffer in struct connection
 */
static enum connection_state receive_message(struct connection *conn)
{
	ssize_t bytes_recv = 0;
	int rc, offset = 0;
	char abuffer[64];
	size_t bytes_parsed;

	rc = get_peer_address(conn->sockfd, abuffer);
	if (rc < 0) {
		ERR("get_peer_address");
		goto remove_connection;
	}

	do {
		bytes_recv =
		recv(conn->sockfd, conn->recv_buffer + offset,
			BUFSIZ - offset, 0);
		if (bytes_recv < 0) { /* Error in communication */
			dlog(LOG_ERR, "Error in communication from: %s\n",
				abuffer);
			goto remove_connection;
		}
		if (bytes_recv == 0) { /* Connection closed */
			dlog(LOG_INFO, "Connection closed from: %s\n", abuffer);
			conn->state = STATE_CONNECTION_CLOSED;
			return STATE_CONNECTION_CLOSED;
		}
		offset += bytes_recv;
		DIE(offset < 0, "EOF");
	} while (is_req_finished(conn->recv_buffer, offset));

	dlog(LOG_INFO, "Received message from: %s\n", abuffer);
	dlog(LOG_DEBUG, "\n-----_ HTTP REQUEST -----\n%s\n", conn->recv_buffer);

	/* Parse path from HTTP request */
	http_parser_init(&request_parser, HTTP_REQUEST);
	memset(request_path, 0, BUFSIZ);
	bytes_parsed =
	http_parser_execute(&request_parser, &settings_on_path,
		conn->recv_buffer, strlen(conn->recv_buffer));

	dlog(LOG_DEBUG, "Parsed simple HTTP request (bytes: %lu), path: %s\n",
		bytes_parsed, request_path);
	memset(conn->request_path, 0, BUFSIZ);
	strcpy(conn->request_path, request_path);
	conn->recv_len = bytes_recv;
	conn->state = STATE_DATA_RECEIVED;

	return STATE_DATA_RECEIVED;

remove_connection:
	dlog(LOG_DEBUG, "Closing communication.\n");
	rc = w_epoll_remove_ptr(epollfd, conn->eventfd, conn);
	DIE(rc < 0, "w_epoll_remove_ptr");

	rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	DIE(rc < 0, "w_epoll_remove_ptr");

	connection_remove(conn);

	return STATE_CONNECTION_CLOSED;
}

/*
 * Read data asynchronously using Linux AIO
 */
static void read_data(struct connection *conn)
{
	int rc;
	struct iocb iocb = {0};
	struct iocb *piocb;

	/* Add evenfd to epoll */
	conn->eventfd = eventfd(0, 0);
	rc = w_epoll_add_ptr_eventfd(epollfd, conn->eventfd, conn);
	DIE(rc < 0, "w_epoll_add_eventd");

	piocb = &iocb;

	io_prep_pread(&iocb, conn->fd, conn->send_buffer, BUFSIZ,
		conn->send_len);

	/* Set up eventfd notification */
	io_set_eventfd(&iocb, conn->eventfd);

	/* Submit aio */
	rc = io_submit(conn->ctx, 1, &piocb);
	DIE(rc < 0, "io_submit failed");
}

/* Write log entry to logfile */
static void write_log(struct utmpx *ut) {
    char format_buf[BUFSIZ];
    int rc;


	if (ut->ut_type == USER_PROCESS ||
	ut->ut_type == LOGIN_PROCESS ||
	ut->ut_type == DEAD_PROCESS ||
	ut->ut_type == RUN_LVL) {
		/* Log
		 * Username
		 */
		memset(format_buf, 0, BUFSIZ);
		sprintf(format_buf, "%-8s ", ut->ut_user);
		rc = write(logfd, format_buf, strlen(format_buf));
		DIE(rc < 0, "write failed");

		/* Log
		 * Type of record
		 */
		memset(format_buf, 0, BUFSIZ);
		sprintf(format_buf, "%-9.9s ",
			(ut->ut_type == EMPTY) ? "EMPTY" :
			(ut->ut_type == RUN_LVL) ? "RUN_LVL" :
			(ut->ut_type == BOOT_TIME) ? "BOOT_TIME" :
			(ut->ut_type == NEW_TIME) ? "NEW_TIME" :
			(ut->ut_type == OLD_TIME) ? "OLD_TIME" :
			(ut->ut_type == INIT_PROCESS) ? "INIT_PR" :
			(ut->ut_type == LOGIN_PROCESS) ? "LOGIN_PR" :
			(ut->ut_type == USER_PROCESS) ? "USER_PR" :
			(ut->ut_type == DEAD_PROCESS) ? "DEAD_PR" : "???");
		rc = write(logfd, format_buf, strlen(format_buf));
		DIE(rc < 0, "write failed");

		/* Log 
		 * PID of login process
		 * Device name of tty - "/dev/"
		 * Terminal name suffix or inittab ID
		 * Hostname for remote login
		 */
		memset(format_buf, 0, BUFSIZ);
		sprintf(format_buf, "%5ld %-6.6s %-3.5s %-20s ", (long) ut->ut_pid,
			ut->ut_line, ut->ut_id, ut->ut_host);
		rc = write(logfd, format_buf, strlen(format_buf));
		DIE(rc < 0, "write failed");

		/* Log timestamp */
		memset(format_buf, 0, BUFSIZ);
		time_t t = (time_t) ut->ut_tv.tv_sec;
		sprintf(format_buf, "%s\n", ctime(&t));
		rc = write(logfd, format_buf, strlen(format_buf));
		DIE(rc < 0, "write failed");

		memset(&ut, 0, sizeof(ut));
    }
}

/*
 * Send data asynchronously using Linux AIO
 */
static void send_data(struct connection *conn)
{
	int rc;
	struct iocb iocb = {0};
	struct iocb *piocb;

	/* Add evenfd to epoll */
	conn->eventfd = eventfd(0, 0);
	rc = w_epoll_add_ptr_eventfd(epollfd, conn->eventfd, conn);
	DIE(rc < 0, "w_epoll_add_eventd");

	/* Prepare to send data */
	piocb = &iocb;
	if (conn->is_header) {
		io_prep_pwrite(&iocb, conn->sockfd, conn->send_buffer,
			conn->header_size, 0);
		conn->is_header = FALSE;
	} else {
		io_prep_pwrite(&iocb, conn->sockfd, conn->send_buffer,
			conn->read_data, 0);
	}

	/* Set up eventfd notification */
	io_set_eventfd(&iocb, conn->eventfd);

	/* Submit aio */
	rc = io_submit(conn->ctx, 1, &piocb);
	DIE(rc < 0, "io_submit failed");
}

/*
 * Handle a client request on a client connection
 */
static void handle_client_request(struct connection *conn)
{
	int rc;
	enum connection_state ret;

	ret = receive_message(conn);
	if (ret == STATE_CONNECTION_CLOSED)
		return;

	/* Add socket to epoll for out events */
	rc = w_epoll_update_ptr_inout(epollfd, conn->sockfd, conn);
	DIE(rc < 0, "w_epoll_add_ptr_inout");
}


/*
 * Handle EPOLLOUT requests on client connection
 */
static void handle_work(struct connection *conn)
{
	int rc, size, fd;

	switch (conn->state) {
	case STATE_DATA_RECEIVED: /* Start sending header */
		if (is_file(conn->request_path)) {
			dlog(LOG_DEBUG, "Sending HTTP header\n");
			fd = open(conn->request_path + 1, O_RDWR, 0644);
			DIE(fd < 0, "open failed");

			size = lseek(fd, 0, SEEK_END);
			DIE(size < 0, "lseek failed");
			rc = lseek(fd, 0, SEEK_SET);
			DIE(rc < 0, "lseek failed");

			write_http200_header(conn->send_buffer, size);
			conn->file_dimension = size;
			conn->data_size = conn->header_size;
			conn->fd = fd;
		} else {
			dlog(LOG_INFO, "Sending HTTP header for 404 Not Found");
			write_http404_header(conn->send_buffer);
		}
		conn->header_size = strlen(conn->send_buffer);
		conn->is_header = TRUE;
		conn->state = STATE_SENDING;
		break;
	case STATE_READING: /* Send file */
			dlog(LOG_DEBUG, "Reading DYNAMIC file\n");
			read_data(conn);
			rc = w_epoll_update_ptr_in(epollfd, conn->sockfd, conn);
			DIE(rc < 0, "w_epoll_add_ptr_in");
		break;
	case STATE_SENDING:
		dlog(LOG_DEBUG, "Request to SEND data.\n");
		send_data(conn);
		rc = w_epoll_update_ptr_in(epollfd, conn->sockfd, conn);
		DIE(rc < 0, "w_epoll_add_ptr_in");
		break;
	default:
		DIE(1, "Unknown state");
	}
}

/*
 * Gets triggered when an async operations finished doing work; considers
 *      * reading from file finished
 *      * sending finished
 */
static void handle_work_done(struct connection *conn)
{
	struct io_event *events;
	int rc;

	/* Remove eventfd */
	rc = w_epoll_remove_ptr(epollfd, conn->eventfd, conn);
	DIE(rc < 0, "w_epoll_remove_ptr");
	close(conn->eventfd);

	events = malloc(sizeof(*events));
	DIE(!events, "malloc failed");

	/* Wait for async operations to finish */
	rc = io_getevents(conn->ctx, 1, 1, events, NULL);
	DIE(rc < 0, "io_getevents failed");

	switch (conn->state) {
	/* Finished reading from file async */
	case STATE_READING:
		dlog(LOG_DEBUG, "Done reading\n");
		conn->send_len += events->res;
		conn->read_data = events->res;

		rc = w_epoll_update_ptr_out(epollfd, conn->sockfd, conn);
		DIE(rc < 0, "w_epoll_add_ptr_out");
		conn->state = STATE_SENDING;
		break;
	/* Finished sending file async */
	case STATE_SENDING:
		dlog(LOG_DEBUG, "Done sending.\n");

		/* Done sending file, closing connection*/
		if (conn->send_len == conn->file_dimension) {
			dlog(LOG_INFO, "Logfile sent succesfully.\n");
			goto remove_connection;
		}
		rc = w_epoll_update_ptr_out(epollfd, conn->sockfd, conn);
		DIE(rc < 0, "w_epoll_add_ptr_out");
		conn->state = STATE_READING;

		break;
	default:
		DIE(1, "Unknown state");
	}

	return;

remove_connection:
	dlog(LOG_DEBUG, "Closing communication.\n");
	rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	DIE(rc < 0, "w_epoll_remove_ptr");

	connection_remove(conn);
}

/*
 * Handle epollin requests; consider
 *      * new connections from clients
 *      * finished async operations
 */
static void handle_epollin(struct connection *conn)
{
	switch (conn->state) {
	case STATE_NEW_CONN:
		dlog(LOG_DEBUG, "New message\n");
		handle_client_request(conn);
		break;
	default:
		dlog(LOG_DEBUG, "Check work done\n");
		handle_work_done(conn);
		break;
	}
}

/* 
* Get data from the OS 
* Format it
* Store it in logfile
*/
static void create_logfile() {
	struct utmpx *ut;

	logfd = open(AUTH_LOG_PATH, O_RDWR | O_CREAT, 0644);
    DIE(logfd < 0, "open failed");

	utmpxname(PATH_WTMP);
	setutxent();
	
	memset(&ut, 0, sizeof(ut));
	while((ut = getutxent()) != NULL) {
		write_log(ut);
	}

	endutxent();
}

/* Check if file exists */
static int file_exists(char *buffer) {
	if (access(buffer + 1, F_OK) != -1)
		return 1;
	return 0;
}

int main(void)
{
	int rc, fd;
	struct utmpx ut;
    char buffer[BUFSIZ];

	/* If logfile doesn't exists, create it */
    if (!file_exists(AUTH_LOG_PATH))
       create_logfile();
    else
		logfd = open(AUTH_LOG_PATH, O_RDWR, 0644);

	dlog(LOG_DEBUG, "\n-----------------SERVER STARTED -----------------\n");

	/* Init multiplexing */
	epollfd = w_epoll_create();
	DIE(epollfd < 0, "w_epoll_create");

	/* Creating the inotify instance*/
    inotifyfd = inotify_init();
    DIE(inotifyfd < 0, "inotify_init failed");

	/* Start monitoring WTMP */
    wtmpfd = inotify_add_watch(inotifyfd, PATH_WTMP, IN_MODIFY);
    DIE(wtmpfd < 0, "inotify_add_watch failed");

	/* Add inotify fd to epoll */
    rc = w_epoll_add_fd_in(epollfd, inotifyfd);
    DIE(rc < 0, "w_epoll_add_fd_in failed");

	/* Create server socket */
	listenfd = tcp_create_listener(IMONITOR_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);
	DIE(listenfd < 0, "tcp_create_listener");

	/* Add socket to epoll */
	rc = w_epoll_add_fd_in(epollfd, listenfd);
	DIE(rc < 0, "w_epoll_add_fd_in");

	dlog(LOG_INFO, "Server waiting for connections on port %d\n",
	     IMONITOR_LISTEN_PORT);

	/* Server main loop */
	while (1) {
		struct epoll_event rev;

		/* Wait for events */
		rc = w_epoll_wait_infinite(epollfd, &rev);
		DIE(rc < 0, "w_epoll_wait_infinite");

		/*
		 * Switch event types; consider
		 *   - new connection requests (on server socket)
		 *   - socket communication (on connection sockets)
		 */
		if (rev.data.fd == listenfd) { /* new connection */
			dlog(LOG_DEBUG, "New connection\n");
			if (rev.events & EPOLLIN)
				handle_new_connection();
		} else if (rev.data.fd == inotifyfd) { /* new log entry */
                rc = read(rev.data.fd, &buffer, BUFSIZ);
                DIE(rc < 0, "read failed");

                fd = open(PATH_WTMP, O_RDONLY, 0644);
                DIE(fd < 0, "open failed");

                lseek(fd, -sizeof(ut), SEEK_END);
                memset(&ut, 0, sizeof(ut));
                rc = read(fd, &ut, sizeof(ut));
                DIE(rc < 0, "read failed");

                write_log(&ut);
		} else if (rev.events & EPOLLOUT) { /* need to send file */
			dlog(LOG_DEBUG, "Ready to do work\n");
			handle_work(rev.data.ptr);
		} else if (rev.events & EPOLLIN) { /* new message OR done doing async operation */
			dlog(LOG_DEBUG, "Received epollin");
			handle_epollin(rev.data.ptr);
		} else {
			DIE(1, "Unknown event");
		}
	}
	return 0;
}
