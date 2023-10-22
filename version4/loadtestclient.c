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
    int rv;
    int numbytes;
    char buffer[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];
    int sockfd, portno, n;
    if(argc!=6){
        printf("usage ./submit  <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <timeout-seconds>");
        exit(0);
    }
    int loopNum=atoi(argv[3]);
    int sleepTime = atoi(argv[4]);
    double timeout = (double)atoi(argv[5])*1.0;
  //Structure for handling Server Internet Address
  struct sockaddr_in serv_addr;
  struct hostent *server; //To store returned value of gethostbyname

  //Command Line Input : Server IP and Port No 
  char* serverIP = strtok(argv[1], ":");
  char* serverPort = strtok(NULL, ":");
  
  portno = atoi(serverPort);

  /*Create socket*/
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  
  if (sockfd < 0)
    perror("ERROR opening socket");

  // Receiving timeout
//   struct timeval tv;
//   tv.tv_sec = timeout;
//   setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));

  /*Fill in Server Address*/
  /*If name is an IPv4 address, no lookup is performed and
    gethostbyname() simply copies name into the h_name field*/	
  server = gethostbyname(serverIP); 

  if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
  }
  bzero((char *)&serv_addr, sizeof(serv_addr)); // initialize serv_address bytes to all zeros
  serv_addr.sin_family = AF_INET; // Address Family of INTERNET
  bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
  serv_addr.sin_port = htons(portno);

  /*Connect To Server*/
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        {
            perror("ERROR connecting");
            exit(0);
        }

    double total_time = 0;
    double timetook = 0;
    struct timeval start,end,start1,mid,rsent,rrecv;
    double reqTime=0;
    gettimeofday(&start,NULL);
    int timeout_count=0;
    for(int iter = 0;iter<loopNum;iter++){
        gettimeofday(&start1,NULL);
        int c_file_fd = open(strdup(argv[2]),O_RDONLY);
        if(c_file_fd<0){
            printf("Unable to open %s\n",argv[2]);
        }
        int n_reads = 0;
        int total = 0;
        int file_size=0;
        while((n_reads=read(c_file_fd,buffer,sizeof buffer))>0){
            file_size+=n_reads;
            printf("%d\n",n_reads);
        }
        printf("File size: %d\n",file_size);
        char fileS[10];
        sprintf(fileS,"%d",file_size);
        send(sockfd,fileS,sizeof fileS,0);
        recv(sockfd,buffer,sizeof buffer,0);
        printf("%s\n",buffer);
        bzero(buffer, sizeof buffer);
        c_file_fd=open(strdup(argv[2]),O_RDONLY);
        gettimeofday(&rsent,NULL);
        while((n_reads = read(c_file_fd,buffer,sizeof buffer))>0){
            //int n_reads = read(c_file_fd,buffer,sizeof buffer);
            total+=n_reads;
            
            if ((numbytes = send(sockfd, buffer, n_reads,0)) == -1) {
                perror("client: sendto");
                exit(1);
            }
            printf("client: sent %d bytes to %s\n", n_reads, argv[1]);
            bzero(buffer,sizeof buffer);
        }
        gettimeofday(&rrecv,NULL);
        double oneReqtimetook = (rrecv.tv_sec - rsent.tv_sec)+
              ((rrecv.tv_usec - rsent.tv_usec)/1.0e6);
        reqTime+=oneReqtimetook;
        struct sockaddr_storage their_addr;
        socklen_t addr_len;
        char recv_buff[1024];
        printf("Waiting for the server response...\n");
        if ((numbytes = recv(sockfd, recv_buff, MAXBUFLEN , 0)) == -1) {
            timeout_count+=1;
            perror("recv");
            exit(1);
        }
        printf("numbytes: %d\n",numbytes);
        recv_buff[numbytes]='\0';
        printf("Server Msg: %s",recv_buff);
        memset(recv_buff,0,sizeof recv_buff);
        gettimeofday(&mid,NULL);     
        timetook = (mid.tv_sec - start1.tv_sec)+
              ((mid.tv_usec - start1.tv_usec)/1.0e6);
        if(timetook>timeout){
            timeout_count+=1;
        }      
        // time_taken = (time_taken + (end.tv_usec -
        //                         start.tv_usec)/1000000.0);
        sleep(sleepTime);
    }
   gettimeofday(&end,NULL);
   double time_taken;
   time_taken=(end.tv_sec - start.tv_sec) + 
              ((end.tv_usec - start.tv_usec)/1.0e6);
    printf("Time Taken: %f\n\n",time_taken);
     
    double avg = time_taken / (loopNum*1.0);
    printf("Timeout Count: %d\n",timeout_count);
    printf("Request Time: %f\n",reqTime);
    printf("Average Response time: %f\n",avg);
    printf("Throughput: %f\n",loopNum/time_taken);
    printf("Goodput: %f",(loopNum-timeout_count)/time_taken);
    send(sockfd, "QUIT", sizeof "QUIT",0);
    n =recv(sockfd, buffer, sizeof buffer,0);
    printf("%d\n",n);
    printf("Server Response: %s\n",buffer);
    close(sockfd);
    printf("Client Exit...");
    return 0;
}