/****************************************************************************
* This is a code written by Prof. Alistair Moffat
* The only functions added are the last 3 functions, made to suit the project
* minor changes had to be done to suit the needs of the project, e.g data_t
* was replaced by work_t
****************************************************************************/

#include "list.h"


list_t
*make_empty_list(void) {
	list_t *list;
	list = (list_t*)malloc(sizeof(*list));
	assert(list!=NULL);
	list->head = list->foot = NULL;
	return list;
}

int
is_empty_list(list_t *list) {
	assert(list!=NULL);
	return list->head==NULL;
}

void
free_list(list_t *list) {
	node_t *curr, *prev;
	assert(list!=NULL);
	curr = list->head;
	while (curr) {
		prev = curr;
		curr = curr->next;
		free(prev);
	}
	free(list);
}


list_t
*insert_at_foot(list_t *list, work_t value) {
	node_t *new;
	new = (node_t*)malloc(sizeof(*new));
	assert(list!=NULL && new!=NULL);
	new->data = value;
	new->next = NULL;
	if (list->foot==NULL) {
		/* this is the first insertion into the list */
		list->head = list->foot = new;
	} else {
		list->foot->next = new;
		list->foot = new;
	}
	return list;
}

work_t
get_head(list_t *list) {
	assert(list!=NULL && list->head!=NULL);
	return list->head->data;
}

list_t
*get_tail(list_t *list) {
	node_t *oldhead;
	assert(list!=NULL && list->head!=NULL);
	oldhead = list->head;
	list->head = list->head->next;
	if (list->head==NULL) {
		/* the only list node just got deleted */
		list->foot = NULL;
	}
	free(oldhead);
	return list;
}

/*
* updates the number of works in the queue
*/
void update_queue_no(list_t *work_queue){
	node_t *node = NULL;
	node = work_queue->head;
	int count = 0;
	while(node){
		node->queue_no = count;
		count += 1;
		node = node->next;
	}
}

/*
*creates a new queue, which only has the works that are not aborted
*/
list_t *clear_queue(list_t *list, int client_id){
	node_t *curr, *prev;
	list_t *new_queue;
	new_queue = make_empty_list();
	assert(new_queue != NULL);
	while(curr){
		prev = curr;
		curr = curr->next;
		if(prev->data.client_sock_id != client_id){
			insert_at_foot(new_queue, prev->data);
		}
	}
	free_list(list);
	update_queue_no(new_queue);
	return new_queue;
}

/*
*sets the abrt flag for works belonging to client with client_socket
*/
void set_work_abrt(list_t *list, int client_socket){
	node_t *curr, *prev;
	assert(list!=NULL);
	curr = list->head;
	while (curr) {
		if(curr->data.client_sock_id == client_socket){
			curr->data.abrt = 1;
		}
		curr = curr->next;
	}
}

/* =====================================================================
   Program written by Alistair Moffat, as an example for the book
   "Programming, Problem Solving, and Abstraction with C", Pearson
   Custom Books, Sydney, Australia, 2002; revised edition 2012,
   ISBN 9781486010974.

   See http://people.eng.unimelb.edu.au/ammoffat/ppsaa/ for further
   information.

   Prepared December 2012 for the Revised Edition.
   ================================================================== */
