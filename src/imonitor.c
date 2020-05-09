#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

/* watch /dev/pts */
#define DIR "/dev/pts/"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

/* Get absolute path */
char *get_path(char *dir, char *file) {
    char *path;
    size_t dir_size = strlen(dir);
    size_t file_size = strlen(file);

    path = malloc(sizeof(char) * (dir_size + file_size + 1));
    strcpy(path, dir);
    strcpy(&(path[dir_size]), file);

    return path;
}

int main(void) {
    int fd, wd, rt;
    char buffer[EVENT_BUF_LEN];
    char *path;
    struct stat status;
    struct passwd *pwd;

    /* Creating the INOTIFY instance*/
    fd = inotify_init();
    DIE(fd < 0, "[inotify_init] failed");

    /*Add DIR into watch list */
    wd = inotify_add_watch(fd, DIR, IN_CREATE | IN_DELETE);
    DIE(wd < 0, "[inotify_add_watch] failed");

    int i = 5;
    while (i--) {
        /* Read to determine the event change happens on “/me” directory */
        rt = read(fd, buffer, EVENT_BUF_LEN);
        DIE(rt < 0, "[read] failed");

        struct inotify_event *event = (struct inotify_event *)buffer;
        path = get_path(DIR, event->name);

        rt = stat(path, &status);
        DIE(rt < 0, "[stat] failed");

        pwd = getpwuid(status.st_uid);
        DIE(pwd == NULL, "[getpwuid] failed");

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

    inotify_rm_watch(fd, wd);
    free(path);
    free(pwd);
    close(fd);
}