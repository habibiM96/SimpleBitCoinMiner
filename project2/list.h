/****************************************************************************
* This is a code written by Prof. Alistair Moffat
* The only functions added are the last 3 functions, made to suit the project
* minor changes had to be done to suit the needs of the project, e.g data_t
* was replaced by work_t
****************************************************************************/

#ifndef LIST_H
#define LIST_H


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "datastruct.h"

typedef struct node node_t;

struct node {
	work_t data;
	int queue_no; // the number of the work in the queue
	node_t *next;
};

typedef struct {
	node_t *head;
	node_t *foot;
} list_t;




list_t *make_empty_list(void);

int is_empty_list(list_t *list);

void free_list(list_t *list);

list_t *insert_at_foot(list_t *list, work_t value);

work_t get_head(list_t *list);

list_t *get_tail(list_t *list);

void update_queue_no(list_t *work_queue);

list_t *clear_queue(list_t *list, int client_id);

void set_work_abrt(list_t *list, int client_socket);

#endif
