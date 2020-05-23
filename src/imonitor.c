/*
 *  imonitor
 *  Program to monitor
 */

#define _GNU_SOURCE

#include "imonitor.h"

#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>
 #include <sys/syscall.h>

#include "debug.h"
#include "sock_util.h"
#include "utils.h"

/* Logfile path */
#define AUTH_LOG_PATH "./auth.log"

/* epoll file descriptor */
static int epollfd;

/* Server socket file descriptor */
static int listenfd;

/* Inotify file descriptor */
static int inotifyfd;

/* WTMP file descriptor */
static int wtmpfd;

/* Logfile file descriptor */
static int logfd;

enum connection_state {
    STATE_DATA_RECEIVED,
    STATE_DATA_SENT,
    STATE_CONNECTION_CLOSED,
    STATE_READING,
    STATE_SENDING,
    STATE_SENT,
    STATE_NEW_CONN
};

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
    size_t header_size;
    size_t data_size;
    size_t is_header;
    size_t read_data;
    int fd;
};

/* Get absolute path */
static char *get_path(char *dir, char *file) {
    char *path;
    size_t dir_size = strlen(dir);
    size_t file_size = strlen(file);

    path = malloc(sizeof(char) * (dir_size + file_size + 1));
    DIE(path == NULL, "[failed] failed");
    strcpy(path, dir);
    strcpy(&(path[dir_size]), file);

    return path;
}

int file_exists(char *buffer)
{
	if (access(buffer + 1, F_OK) != -1)
		return 1;
	return 0;
}

/* Format and write log to file */
static void write_log(struct utmpx *ut) {
    char format_buf[BUFSIZ];
    int rc;

    memset(format_buf, 0, BUFSIZ);
		if (ut->ut_type == USER_PROCESS ||
		    ut->ut_type == LOGIN_PROCESS ||
		    ut->ut_type == DEAD_PROCESS ||
            ut->ut_type == RUN_LVL) {
			sprintf(format_buf, "%-8s ", ut->ut_user);
			printf("%-8s ", ut->ut_user);
			rc = write(logfd, format_buf, strlen(format_buf));
			DIE(rc < 0, "write failed");
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
			printf("%-9.9s ",
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
			memset(format_buf, 0, BUFSIZ);
			sprintf(format_buf, "%5ld %-6.6s %-3.5s %-20s ", (long) ut->ut_pid,
				ut->ut_line, ut->ut_id, ut->ut_host);
			printf("%5ld %-6.6s %-3.5s %-20s ", (long) ut->ut_pid,
				ut->ut_line, ut->ut_id, ut->ut_host);
			rc = write(logfd, format_buf, strlen(format_buf));
			DIE(rc < 0, "write failed");
			memset(format_buf, 0, BUFSIZ);
            time_t t = (time_t) ut->ut_tv.tv_sec;
			sprintf(format_buf, "%s\n", ctime(&t));
            printf("%s\n", ctime(&t));
			rc = write(logfd, format_buf, strlen(format_buf));
			DIE(rc < 0, "write failed");
			memset(&ut, 0, sizeof(ut));
    }
}

/* 
* Get data from the OS 
* Format it
* Store it in logfile
*/
static void create_logfile() {
	int fd, rc;
	struct utmpx *ut;

	logfd = open(AUTH_LOG_PATH, O_RDWR | O_CREAT, 0644);
    DIE(logfd < 0, "open failed");

	utmpxname(PATH_WTMP);
	setutxent();
	
	// sprintf(format_buf, "user 	type 	PID     line 	id 		host 	date/time\n");
	// rc = write(logfd, format_buf, strlen(format_buf));
	// DIE(rc < 0, "write failed");
	memset(&ut, 0, sizeof(ut));
	while((ut = getutxent()) != NULL) {
		write_log(ut);
	}

	endutxent();
}

int main(int argc, char *argv[]) {
    int rc, fd, logfd;
	struct utmpx *ut = NULL;
    char buffer[BUFSIZ];
	char format_buf[BUFSIZ];

    /* If logfile doesn't exists, create it */
    if (!file_exists(AUTH_LOG_PATH)) {
       create_logfile();
    } else { /* Else update logfile */
    }

    /* Init multiplexing */
    epollfd = w_epoll_create();
    DIE(epollfd < 0, "w_epoll_create failed");

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
    listenfd =
        tcp_create_listener(IMONITOR_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);
    DIE(listenfd < 0, "tcp_create_listener");

    /* Add socket to epoll */
    rc = w_epoll_add_fd_in(epollfd, listenfd);
    DIE(rc < 0, "w_epoll_add_fd_in");

    /* Server main loop */
    while (1) {
        struct epoll_event rev;

        /* Wait for events */
        memset(&rev, 0, sizeof(rev));
        rc = w_epoll_wait_infinite(epollfd, &rev);
        DIE(rc < 0, "w_epoll_wait_infinite");

        if (rev.events & EPOLLIN) {
            if (rev.data.fd == inotifyfd) {
                printf("Am primit EPOLLIN!\n");
                rc = read(rev.data.fd, &buffer, BUFSIZ);
                DIE(rc < 0, "read failed");

                int fd, rc;
                struct utmpx ut;

                fd = open(PATH_WTMP, O_RDONLY, 0644);
                DIE(fd < 0, "open failed");

                lseek(fd, -sizeof(ut), SEEK_END);
                memset(&ut, 0, sizeof(ut));
                rc = read(fd, &ut, sizeof(ut));
                DIE(rc < 0, "read failed");

                write_log(&ut);
            }
        }
    }
}
