/*
 * Hashtable implemented with linked lists
 *
 * Elements are a node structure containing:
 *      file = contains id and inode info of the file
 *      next = next element in the list
 */
#include "hashtable.h"

#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils.h"

/* watch /dev/pts */
#define DIR "/dev/pts/"

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

struct node *hmap_init() {
    return NULL;
}

/* Insert at the head of the list */
void hmap_insert(struct node **head, char *filename) {
    struct node *new;
    char *path = NULL;
    struct stat status;
    struct passwd *pwd = NULL;
    int rt;

    /* Find user */
    path = get_path(DIR, filename);
    rt = stat(path, &status);
    DIE(rt < 0, "[stat] failed");
    free(path);
    pwd = getpwuid(status.st_uid);
    DIE(pwd == NULL, "[getpwuid] failed");

    /* Insert */
    new = malloc(sizeof(*new));
    DIE(new == NULL, "[malloc] failed");

    new->file.filename = strdup(filename);
    new->file.uid = status.st_uid;
    new->file.username = strdup(pwd->pw_name);

    /* If hashtable is empty */
    if (!(*head)) {
        *head = new;
        return;
    }

    new->next = *head;
    *head = new;
}

char *hmap_get_user(struct node **head, char *filename) {
    struct node *current = *head;

    do {
        if (!strcmp(current->file.filename, filename)) break;
        current = current->next;
    } while (current);

    return current->file.username;
}

/*
 * -1 --> failed
 */
int hmap_delete(struct node **head, char *filename) {
    struct node *current = *head;
    struct node *prev = NULL;

    if (*head == NULL) return -1;

    do {
        if (!strcmp(current->file.filename, filename)) break;
        prev = current;
        current = current->next;
    } while (current->next);

    /* Node is first */
    if (current == *head) {
        *head = current->next;
        goto free;
    }

    /* Node is last */
    if (current->next == NULL) {
        prev->next = NULL;
        goto free;
    }

    prev->next = current->next;

free:
    free(current->file.filename);
    free(current->file.username);
    free(current);
    return 0;
}

void hmap_destroy(struct node **head) {
    struct node *tmp = NULL;
    struct node *node = *head;

    if (node != NULL) {
        do {
            tmp = node;
            node = node->next;
            free(tmp->file.filename);
            free(tmp->file.username);
            free(tmp);
        } while (node);
    }
}

void hmap_print(struct node **head) {
    struct node *current = *head;
    do {
        printf("%s ", current->file.username);
        current = current->next;
    } while (current);
    printf("\n");
}

// int main() {
//     struct node *head = hmap_init();
//     hmap_insert(&head, "1");
//     hmap_insert(&head, "2");
//     hmap_insert(&head, "3");
//     hmap_insert(&head, "4");
//     hmap_insert(&head, "5");
//     hmap_insert(&head, "6");
//     // hmap_print(&head);
//     hmap_destroy(&head);
//     // hmap_delete(&head, "1");
//     // hmap_print(&head);
//     return 0;
// }
