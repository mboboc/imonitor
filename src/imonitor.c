#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

int main(void)
{
    int fd, wd, rt;
    char buffer[EVENT_BUF_LEN];
    struct stat status;
    struct passwd *pwd;

    /* Creating the INOTIFY instance*/
    fd = inotify_init();
    DIE(fd < 0, "[inotify_init] failed");

    /*Adding the “me” directory into watch list */
    wd = inotify_add_watch(fd, "/dev/pts", IN_CREATE | IN_DELETE);
    DIE(wd < 0, "[inotify_add_watch] failed");

    while (1) {
        /* Read to determine the event change happens on “/me” directory */
        rt = read(fd, buffer, EVENT_BUF_LEN);
        DIE(rt < 0, "[read] failed");

        struct inotify_event *event = (struct inotify_event *)&buffer[0];
        if (event->mask & IN_CREATE) {
            if (event->mask & IN_ISDIR)
                printf("New directory %s created.\n", event->name);
            else
                printf("New file %s created.\n", event->name);
        } else if (event->mask & IN_DELETE) {
            if (event->mask & IN_ISDIR)
                printf("Directory %s deleted.\n", event->name);
            else
                printf("File %s deleted.\n", event->name);
        }
        stat(event->name, &status);
        pwd = getpwuid(status.st_uid);
        printf("%s\n", pwd->pw_name);
        printf("Finished...\n");
    }

    /* The directory from the watch list.*/
    inotify_rm_watch(fd, wd);

    /*closing the INOTIFY instance*/
    close(fd);
}