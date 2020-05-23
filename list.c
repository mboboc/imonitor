/*
 * Linked list implementation
 */

#include "list.h"

struct node *init_list(void) {
    return NULL;
}

/* Insert at the begining of list */
void insert_list(struct node **head, struct utmpx *ut) {
    struct node *new;

    new = malloc(sizeof(*new));
    DIE(new == NULL, "malloc failed");

    memcpy(new->ut, ut, sizeof(ut));
    new->next = NULL;

    if (*head == NULL) {
        *head = new;
        return;
    }
    
    new->next = *head;
    *head = new;
}

/* Delete after PID */
void delete_node(struct node **pp, pid_t pid) {
    while (*pp) {
		if ((*pp)->ut->ut_pid == pid) {
			struct node *victim = *pp;
			*pp = victim->next;
            free(victim->ut);
			free(victim);
		} else {
			pp = &(*pp)->next;
		}
	}
}

/* Print list, used for debugging */
void print_list(struct node **head) {
    struct node *current;

    current = *head;

    do {
        printf("Node: %s with pid = %d\n", current->ut->ut_user, current->ut->ut_pid);
        current = current->next;
    } while (current);
}