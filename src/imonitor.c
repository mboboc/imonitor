#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include "utils.h"

#define EVENT_SIZE      (sizeof(struct inotify_event))
#define EVENT_BUF_LEN   (1024 * (EVENT_SIZE + 16))

int main(void)
{
    int length, i = 0;
    int fd;
    int wd;
    char buffer[EVENT_BUF_LEN];

    /* Creating the INOTIFY instance*/
    fd = inotify_init();
    DIE(fd < 0, "[inotify_init] failed");

    /*Adding the “me” directory into watch list */
    wd = inotify_add_watch(fd, "me", IN_CREATE | IN_DELETE);
    DIE(wd < 0, "[inotify_add_watch] failed");

    /* Read to determine the event change happens on “/me” directory */
    length = read(fd, buffer, EVENT_BUF_LEN);
    DIE(fd < 0, "[read] failed");

    /*
     * Actually read return the list of change events happens
     * Read the change event one by one and process it accordingly
     */
    while (i < length) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        if (event->len) {
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
        }
        printf("Finished...\n");
        i += EVENT_SIZE + event->len;
    }

    /* Removing the “me” directory from the watch list.*/
    inotify_rm_watch(fd, wd);

    /*closing the INOTIFY instance*/
    close(fd);
}