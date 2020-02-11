#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE_THREAD_POOL 30
pthread_t thread_pool[SIZE_THREAD_POOL]; 
char buffer[1024];

sem_t buff, full, thread, mutex;


char interface[] = "HTTP/1.1 200 Ok\n"
"Content-Type: text/html; charset=UTF-8\n\n"
"<!DOCTYPE html>\n"
"<html><head><title>Web Server Application</title>\n"
"<style>body {background-color: darkseagreen;}\n" 
"h1 {color: black;}</style></head><br><br>\n"
"<body><center><h1>This is a Multithreaded Web Server <br> Created by Quoc Anh</h1><br>\n"
"<img src=\"pic1.png\"/>\r\n"
"<img src=\"pic2.png\"/>\r\n"
"<img src=\"pic3.png\"/>\r\n"
"<img src=\"pic4.png\"/>"
"</center></body></html>\n";


int sockfd, newsockfd, portno, threadno, bufno;
int handling_thread = 0;
int handling_queue = 0;
char *schedalg;

struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen = sizeof(cli_addr);


void error(const char *msg)
{
	perror(msg);
	exit(1);
}

//linked list data structure that serve the FIFO policy
struct node
{ 
	int *cli_socket;
	struct node *next;
	int info;
};

typedef struct node node_t;

node_t* head = NULL;
node_t* tail = NULL;

void enqueue(int input)
{
	node_t *newnode = malloc(sizeof(node_t));
   
    newnode->next = NULL;
    newnode->info = input;

    if (handling_queue == 0) /*empty queue*/
    {
        head = newnode;

    } else 
    {
        tail->next = newnode; 
    }
    tail = newnode;
    handling_queue++;
	printf("\nInserting done. Queue length: %d\n",handling_queue);
    return;
}


int dequeue()
{
	int info;
    struct node* temp;
	if(handling_queue>0){
        info = head->info;
        temp = head;
        head = head->next;
        free(temp);
        handling_queue--;
        printf("\nDone. Queue length: %d\n", handling_queue);
        return info;
    }
    else{
		printf("Queue is empty\n");
		return -1;
	}
}

void *main_thread(void *arg);
void *thread_function(void *);
void *httpWorker(void *);
unsigned long fsize(char*);

int main(int argc, char *argv[])
{
	pthread_t main, scheduling;
	portno = atoi(argv[1]);
	threadno = atoi(argv[2]);
	bufno = atoi(argv[3]);
	schedalg = argv[4];
	schedalg = "FIFO"; // We choose FIFO (First In First Out for this project)


	if (argc < 5)
	{
		fprintf(stderr, "ERROR, there are not sufficient arguments, please enter more\n");
		fprintf(stderr, "argument is: %d\n", argc); //this line for test
		exit(1);
	}

	if (threadno <= 0)
	{
		fprintf(stderr, "ERROR, the number of worker threads must be a positive integer\n");
		exit(1);
	}

	if (bufno <= 0)
	{
		fprintf(stderr, "ERROR, the number of request connections must be a positive integer");
		exit(1);
	}

	
	sem_init(&mutex, 0, 1);
    sem_init(&full,0,0);
    sem_init(&buff, 0, bufno);
    sem_init(&thread, 0, threadno);


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
		error("ERROR opening socket");
	}
    
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
        close(sockfd);
        exit(1);
    }

    //Creating thread
	pthread_create(&main, NULL, main_thread, &sockfd);
    pthread_create(&scheduling, NULL, thread_function, NULL);

	pthread_join(main, NULL);
	pthread_join(scheduling, NULL); 

	return 0;
}


void *main_thread(void *arg)
{
    int sockfd = *((int *)arg);
    listen(sockfd, bufno);
    printf("server listen on port: %d\n", portno);

    while (1)
    {   
        if(handling_queue == bufno) printf("Queue is full, wait for processing...\n");
        sem_wait(&buff);

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0)
            fprintf(stderr, "ERROR on accept, fail to connect\n");
        else
            printf("start accept new connection: %d. Inserting into the pending queue.\n", newsockfd);
        
		
        sem_wait(&mutex);
        enqueue(newsockfd);
        sem_post(&mutex);
        sem_post(&full);
    }
    return NULL;
}

void *thread_function(void *arg)
{
    pthread_t worker_thread;
    int p_client;              

    while(1) {
        sem_wait(&full);
        

        if(handling_thread == threadno)
            printf("All threads are busy\n");
      
        sem_wait(&thread);

        sem_wait(&mutex);
            printf("Ready for choosing a thread from the ready queue\n");
        p_client = dequeue();

        sem_post(&mutex);

        //Initialize worker thread to handle the http request 

        if(pthread_create(&worker_thread, NULL, httpWorker, &p_client) != 0)
            fprintf(stdout,"create worker thread failed\n");
        else {
            handling_thread++;
            fprintf(stdout,"Creating worker thread successfully, %d thread running\n",handling_thread);
        }
        sem_post(&buff);
    }
    return NULL;
}

void *httpWorker(void *p_sockfd) {//sockfd contains all the information
    int image;
    int newsockfd = *((int*)p_sockfd); //create a local variable for newsockfd

    bzero(buffer, 1024);
    if(recv(newsockfd, buffer, 1023, 0) < 0)
        fprintf(stderr, "ERROR, cannot receive data\n");
    
    else if(!strncmp(buffer, "GET /favicon.ico", 16)) { //handling request
        image = open("pic1.png", O_RDONLY);
        sendfile(newsockfd, image, NULL, fsize("favicon.ico"));
        close(image);
    }

    else if(!strncmp(buffer, "GET /pic1.png", 14)) {
        image = open("pic1.png", O_RDONLY);
        sendfile(newsockfd, image, NULL, fsize("pic1.png"));
        close(image);
    }

    else if(!strncmp(buffer, "GET /pic2.png", 14)) {
        image = open("pic2.png", O_RDONLY);
        sendfile(newsockfd, image, NULL, fsize("pic2.png"));
        close(image);
    }

    else if(!strncmp(buffer, "GET /pic3.png", 14)) {
        image = open("pic3.png", O_RDONLY);
        sendfile(newsockfd, image, NULL, fsize("pic3.png"));
        close(image);
    }

    else if(!strncmp(buffer, "GET /pic4.png", 14)) {
        image = open("pic4.png", O_RDONLY);
        sendfile(newsockfd, image, NULL, fsize("pic4.png"));
        close(image);
    }

    else {
        send(newsockfd, interface, strlen(interface), 0);
    }
    

	close(newsockfd);

    sleep(6); //set each worker thread to pause 6s for testing

    sem_post(&thread); //semaphore unlock, telling number of thread is not max 
    handling_thread--;

	printf("worker thread finished\n________\n");
    pthread_exit(NULL);
    return NULL;
}

unsigned long fsize(char* file) { //function to calculate file size
    FILE *f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(f);
    fclose(f);

    return len;
}