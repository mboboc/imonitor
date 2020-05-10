#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "hashtable.h"
#include "utils.h"

/* watch /dev/pts */
#define DIR "/dev/pts/"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

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

int main(void) {
    struct node *head = hmap_init();
    int fd, wd, rt;
    char buffer[EVENT_BUF_LEN];
    char *path = NULL;
    struct stat status;
    struct passwd *pwd = NULL;
    time_t t = time(NULL);
    char *timestamp;

    /* Creating the INOTIFY instance*/
    fd = inotify_init();
    DIE(fd < 0, "[inotify_init] failed");

    /* Start monitoring /dev/pts */
    wd = inotify_add_watch(fd, DIR, IN_CREATE | IN_DELETE);
    DIE(wd < 0, "[inotify_add_watch] failed");

    int i = 4;
    while (i--) {
        rt = read(fd, buffer, EVENT_BUF_LEN);
        DIE(rt < 0, "[read] failed");

        struct inotify_event *event = (struct inotify_event *)buffer;
        if (event->mask & IN_CREATE) {
            if (event->mask & IN_ISDIR) {
                printf("New directory %s created.\n", event->name);
            } else {
                /* File created */
                hmap_insert(&head, event->name);
                timestamp = strdup(ctime(&t));
                timestamp[strlen(timestamp) - 1] = '\0';
                printf("%s:%s logged in.\n", timestamp,
                       hmap_get_user(&head, event->name));
            }
        } else if (event->mask & IN_DELETE) {
            if (event->mask & IN_ISDIR)
                printf("Directory %s deleted.\n", event->name);
            else {
                /* File deleted */
                printf("%s:%s logged out.\n", timestamp,
                       hmap_get_user(&head, event->name));
                hmap_delete(&head, event->name);
            }
        }
    }

    if (head != NULL) hmap_destroy(&head);
    inotify_rm_watch(fd, wd);
    close(fd);
}
