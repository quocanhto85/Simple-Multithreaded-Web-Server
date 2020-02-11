#include "queue.h"
#include <stdlib.h>

node_t* head = NULL;
node_t* tail = NULL;

void enqueue(int data)
{
    struct node* temp;

    temp = (struct node *)malloc(1*sizeof(struct node));
    if(temp == NULL){
        printf("Error: couldn't allocate memory\n");
        exit(1);
    }
    temp->ptr = NULL;
    temp->info = data;

    if (handling_queue == 0) /*empty queue*/
    {
        head = temp;

    }else 
    {
        tail->ptr = temp; 
    }
    tail = temp;
    handling_queue++;
	printf("\tInserting done, Queue length: %d\n",handling_queue);
    return;
}


int dequeue()
{
	int info;
    struct node* temp;
	if(handling_queue>0){
        info = head->info;
        temp = head;
        head = head->ptr;
        free(temp);
        handling_queue--;
        printf("\tdone, Queue length: %d\n",handling_queue);
        return info;
    }
    else{
		printf("queue empty\n");
		return -1;
	}
}