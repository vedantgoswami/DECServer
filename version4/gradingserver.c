#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include<pthread.h>
#include <sys/types.h>
#define MYPORT "4950"// the port users will be connecting to
#define BACKLOG 10// how many pending connections queue will hold
#include "thread_struct.h"
#include "server.h"

#define QUEUE_SIZE 10000
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_empty = PTHREAD_COND_INITIALIZER;
int front = 0, rear = 0,taskCount = 0;
struct Pair *reqQueue[QUEUE_SIZE];


void queue_insert(struct Pair *item)
{
    rear = (rear + 1) % QUEUE_SIZE;
    if (front == rear)
    {
        printf("\nOverflow\n");
        if (rear == 0)
            rear = QUEUE_SIZE - 1;
        else
            rear--;
    }
    else
    {
        reqQueue[rear] = item;
    }
}

struct Pair* queue_delete()
{
    struct Pair* item;
    if (front == rear)
    {
        printf("\nThe Queue is empty\n");
    }
    else
    {
        front = (front + 1) % QUEUE_SIZE;
        item = reqQueue[front];
        return item;
    }
}

void *handle_client_request(void *arg){
    struct Pair *item;
    while(1){
        int found = 0;
        pthread_mutex_lock(&queue_lock);
        if(taskCount>0){
            found = 1;
            item = queue_delete();
            taskCount--;
        }
        else{
            pthread_cond_wait(&queue_empty,&queue_lock);
        }
        
        pthread_mutex_unlock(&queue_lock);
        if(found == 1){
                    evaluation(item);
                }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage ./server  <port>  <thread_pool_size>\n");
        exit(1);
    }
    int sockfd, portno, numbytes ,thread_pool_size;
    char buffer[MAXBUFLEN];
    thread_pool_size = atoi(argv[2]);
    pthread_t thread[thread_pool_size];
    for (int i = 0; i < thread_pool_size; i++) {
        if (pthread_create(&thread[i], NULL, handle_client_request, NULL) < 0) {
            perror("ERROR creating thread");
            exit(EXIT_FAILURE);
        }
    }
    portno = atoi(argv[1]);
	/*Create Socket*/
	sockfd = socket(AF_INET,SOCK_STREAM,0);
    char s[INET6_ADDRSTRLEN];
	if(sockfd<0)
		perror("Error Opening Socket");

	//Structure for handling Server Internet Address
	struct sockaddr_in serv_addr,their_addr;
    socklen_t addr_len;
	bzero((char *)&serv_addr, sizeof(serv_addr)); // initialize serv_address bytes to all zeros
	serv_addr.sin_family = AF_INET; // Address Family of INTERNET
  	serv_addr.sin_addr.s_addr = INADDR_ANY;  //Any IP address. 
  	serv_addr.sin_port = htons(portno);

  	 /* bind socket to this port number on this machine */
     if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR on binding");
 
	 /* listen for incoming connection requests */
  	listen(sockfd, 100); // 1 means 1 connection requests can be in queue. 
  	//now server is listening for connections

  	//Structure for handling Client Internet Address
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);

	printf("Server Rolled\n");
     

    int client_id = 0;
    while(1){
        printf("server: Waiting for the client...\n");
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_len);
        client_id++;
        inet_ntop(AF_INET,&(their_addr.sin_addr),s, sizeof s);
        printf("server: Got connection from %s:%d\n",s,client_id); 
        pthread_t thrd_id;
        struct Pair *pair = (struct Pair *)malloc(sizeof(struct Pair));
        if (pair == NULL) {
            perror("Failed to allocate memory for Pair struct");
            exit(EXIT_FAILURE);
        }
        pair->client_id = client_id;
        pair->fd = new_fd;
        pthread_mutex_lock(&queue_lock);
        queue_insert(pair); 
        taskCount++;
        pthread_cond_signal(&queue_empty);
        pthread_mutex_unlock(&queue_lock);
        // printf("main %d %d\n",client_id,new_fd);     
    }
    printf("Server Exited");
    close(sockfd);
    return 0;
}