/*
 * iMonitor via webserver
 * 
 * Header file
 */
#ifndef AWS_H_
#define AWS_H_		1

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <libaio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>
#include <paths.h>
#include <sys/inotify.h>
#include <pwd.h>
#include <time.h>       /* time_t, time, ctime */

#define PATH_UTMP _PATH_UTMP
#define PATH_WTMP _PATH_WTMP
#define TRUE 1
#define FALSE 0

#define IMONITOR_LISTEN_PORT 8888

/* Logfile path */
#define AUTH_LOG_PATH "./auth.log"

#endif /* AWS_H_ */
