#ifndef QUEUE_H
#define QUEUE_H

struct node
{ 
	int info;
	struct node *ptr;
};

typedef struct node node_t;

void enqueue(int data);
int dequeue();
#endif 