#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdlib.h>

struct ifile {
    char *filename;
    int uid;
    char *username;
};

struct node {
    struct ifile file;
    struct node *next;
};

struct node *hmap_init();
void hmap_insert(struct node **head, char *filename);
char *hmap_get_user(struct node **head, char *filename);
int hmap_delete(struct node **head, char *filename);
void hmap_destroy(struct node **head);
void hmap_print(struct node **head);

#endif /* HASHTABLE_H */
