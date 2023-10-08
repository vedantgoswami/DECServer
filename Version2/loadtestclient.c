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
#include<sys/time.h>
#define SERVERPORT "4950"
#define MAXBUFLEN 2048

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);

}

// the port users will be connecting to
int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char buffer[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];
    if (argc != 5) {
        fprintf(stderr,"usage: client hostname message\n");
        exit(1);
    }
    int loopNum = atoi(argv[3]);
    int sleepTime = atoi(argv[4]);
    printf("%d %d",loopNum,sleepTime);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP connection

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and make connect to first
     for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family,get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
    printf("client: connecting to %s\n ", s);
    freeaddrinfo(servinfo); // all done with this structure
    // if ((numbytes = recv(sockfd, buffer, MAXBUFLEN-1, 0)) == -1) {
    //     perror("recv");
    //     exit(1);
    // }
    double total_time = 0;
    struct timeval start,end;
    for(int iter = 0;iter<loopNum;iter++){
        int c_file_fd = open(strdup(argv[2]),O_RDONLY);
        if(c_file_fd<0){
            printf("Unable to open %s\n",argv[2]);
        }
        int n_reads = 0;
        int total = 0;
        gettimeofday(&start,NULL);
        while((n_reads = read(c_file_fd,buffer,sizeof buffer))>0){
            //int n_reads = read(c_file_fd,buffer,sizeof buffer);
            total+=n_reads;
            
            if ((numbytes = send(sockfd, buffer, n_reads,0)) == -1) {
                perror("client: sendto");
                exit(1);
            }
            printf("client: sent %d bytes to %s\n", n_reads, argv[1]);
        }
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
        char recv_buff[1024];
        printf("Waiting for the server response...\n");
        if ((numbytes = recv(sockfd, recv_buff, 1024-1 , 0)) == -1) {
            perror("recv");
            exit(1);
        }
        gettimeofday(&end,NULL);
        recv_buff[numbytes]='\0';
        printf("%s",recv_buff);
        memset(recv_buff,0,sizeof recv_buff);
        double time_taken;
        // time_taken = (end.tv_sec - start.tv_sec);
        // time_taken = (time_taken + (end.tv_usec -
        //                         start.tv_usec)/1000000.0);
        time_taken=(end.tv_sec - start.tv_sec) + 
              ((end.tv_usec - start.tv_usec)/1.0e6);
        total_time+=time_taken;
        printf("Time Taken: %f\n\n",time_taken);
        sleep(sleepTime);
   
   }
    double avg = total_time / (loopNum*1.0);
    printf("Average Response time: %f\n",avg);
    printf("Throughput: %f",loopNum/total_time);
    send(sockfd, "QUIT", sizeof "QUIT",0);
    close(sockfd);
    return 0;
    }