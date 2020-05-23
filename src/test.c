#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>
#include "utils.h"

#define PATH_UTMP _PATH_UTMP
#define PATH_WTMP _PATH_WTMP

int main() {
    int fd, rc;
    struct utmpx ut;

    fd = open(PATH_WTMP, O_RDWR, 0644);
    DIE(fd < 0, "open failed");

    lseek(fd, -sizeof(ut), SEEK_END);
    rc = read(fd, &ut, sizeof(ut));
    DIE(rc < 0, "read failed");

    printf("User = %s\n", ut.ut_user);
    printf("Host = %s\n", ut.ut_host);
    return 0;
}