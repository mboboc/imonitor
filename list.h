#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>

struct node {
    struct utmpx *ut;
	struct node *next;
};

struct node *init_list(void);
void insert_list(struct node **head, struct utmpx *ut);
void delete_node(struct node **pp, pid_t pid);
void print_list(struct node **head);

#endif