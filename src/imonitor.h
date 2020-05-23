#ifndef IMONITOR_H
#define IMONITOR_H

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utmpx.h>
#include <paths.h>

#include "hashtable.h"
#include "utils.h"
#include "w_epoll.h"
#include "sock_util.h"

#define PATH_UTMP _PATH_UTMP
#define PATH_WTMP _PATH_WTMP

#define IMONITOR_LISTEN_PORT 8888

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

#endif /* IMONITOR_H*/