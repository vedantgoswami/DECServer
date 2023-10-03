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
#define MYPORT "4950"// the port users will be connecting to
#define BACKLOG 10// how many pending connections queue will hold


#define MAXBUFLEN 2048

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int compile_file(int id){
    
    int stderr_fd = dup(2); // Making copy of STDERR
    close(2); // Closing STDERR
    char err_name[100];
    sprintf(err_name,"./dummy/error%d%s",id,".txt");
    int err_fd=open(err_name,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    write(err_fd,"COMPILATION ERROR...\n",sizeof "COMPILATION ERROR...\n"-1);
    char exec[100];
    sprintf(exec,"gcc -o ./dummy/recved%d ./dummy/recved%d.c",id,id);
    int compileResult = system(exec);
    close(err_fd);
    dup2(stderr_fd,2); //Restoring STDERR;
    if (compileResult == 0) {
        return 0;
    } 
    return 1;
}
int execute_file(int id){
    // int out_fd=open("out.txt",O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    // dup2(out_fd,1);
    int stdout_fd = dup(1);
    int stderr_fd = dup(2);
    close(2);
    char err_name[100];
    sprintf(err_name,"./dummy/error%d%s",id,".txt");
    int err_fd=open(err_name,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    write(err_fd,"RUNTIME ERROR\n",sizeof "RUNTIME ERROR\n"-1);
    close(1);
    char out_name[100];
    sprintf(out_name,"./dummy/out%d%s",id,".txt");
    int out_fd=open(out_name,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    //  write(1,"Yeh server par nahi jana chaiye",sizeof "Yeh server par nahi jana chaiye"-1);
    char exec[100];
    sprintf(exec,"./dummy/recved%d",id);
    int execResult = system(exec);
    
    //printf("execresult: %d",execResult);
    // close(out_fd);
    close(out_fd);
    close(err_fd);
    dup2(stdout_fd,1);
    dup2(stderr_fd,2);
    return 1;
}
int compare(char *f1, char f2[100]){
    int file1 = open(f1,O_RDONLY);
    int file2 = open(f2,O_RDONLY);
    int rd1,rd2;
    char ch1[1],ch2[1];
    char buf[MAXBUFLEN];
    int n1 = read(file1,buf,sizeof buf);
    int n2 = read(file2,buf,sizeof buf);
    if(n1!=n2 && n1+1!=n2)
        return -1;
    while((rd1=read(file1,ch1,1))>0 && (rd2=read(file2,ch2,1))>0){
        if(ch1[0]!=ch2[0]){
            return -1;
        }
    }
    return 1;
}
int main(void)
{
    int sockfd,new_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sigaction sa;
    int yes=1;
    struct sockaddr_storage their_addr;
    char buffer[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char my_addr[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    } 

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)servinfo->ai_addr;
    inet_ntop(ipv4->sin_family,&(ipv4->sin_addr),my_addr, sizeof my_addr);
    printf("server: %s\n",my_addr);
    
    addr_len = sizeof their_addr;
    int client_id = 0;
    while(1){
        printf("server: Waiting for the client...\n");
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_len);
        client_id++;
        inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
        printf("server: Got connection from %s\n",s); 
        
        if (!fork()) { // this is the child process
                close(sockfd); // child doesn't need the listener
                while(1){
                int total = 0;
                char filename[100];
                sprintf(filename,"./dummy/recved%d.c",client_id);
                
                int c_file_fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                while((numbytes = recv(new_fd, buffer, MAXBUFLEN,MSG_DONTWAIT)) >0) {
                        total+=numbytes;
                    if(numbytes==5){
                        printf("Client Clossed connection...\n");
                        char cmd[100];
                        memset(cmd,'\0',sizeof cmd);
                        sprintf(cmd,"rm -f ./dummy/error%d.txt",client_id);
                        system(cmd);
                        memset(cmd,'\0',sizeof cmd);
                        sprintf(cmd,"rm -f ./dummy/out%d.txt",client_id);
                        system(cmd);
                        memset(cmd,'\0',sizeof cmd);
                        sprintf(cmd,"rm -f ./dummy/recved%d",client_id);
                        system(cmd);
                        memset(cmd,'\0',sizeof cmd);
                        sprintf(cmd,"rm -f ./dummy/recved%d.c",client_id);
                        system(cmd);
                        close(new_fd);
                        exit(0);
                        }
                    printf("server: got packet from %s\n",inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));
                    printf("server: packet is %d bytes long\n", numbytes);
                    //printf("listener: packet contains \"%s\"\n", buf);
                    
                    
                    // Saving the recived data as c file
                    write(c_file_fd,buffer,numbytes);
                    memset(buffer,'\0',sizeof buffer);
                }
                close(c_file_fd);
                
                    //Compiling the recived c file.
                if(total!=0){
                    
                    int compilationStatus = compile_file(client_id);
                    if(compilationStatus==1){ // Compilation Failure
                        printf("Compilation Failed!\n");
                        char err_buff[256];
                        char err_name[100];
                        sprintf(err_name,"./dummy/error%d.txt",client_id);
                        int err_fd = open(err_name,O_RDONLY);
                        int n_read = read(err_fd,err_buff, sizeof err_buff);
                        struct sockaddr *sa = (struct sockaddr *)&their_addr;
                        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                        struct addrinfo hints, *client, *p;
                        memset(&hints, 0, sizeof hints);
                        hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
                        hints.ai_socktype = SOCK_DGRAM;
                        char port[10];
                        sprintf(port,"%d",sin->sin_port);
                        getaddrinfo(inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s), port, &hints, &client);
                        sleep(1);
                        if ((numbytes = send(new_fd, err_buff, n_read,0)) == -1) {
                            perror("server: send");
                            exit(1);
                        }
                        printf("Number of byte sent: %d\n",numbytes);
                        // system("rm -f error.txt");
                    }
                    else{  //Successfully Compiled, Checking for the runtime
                        //printf("Compilation Success...\n");
                        int execResult = execute_file(client_id);
                        char err_buff[MAXBUFLEN];
                        char err_name[100];
                        sprintf(err_name,"./dummy/error%d.txt",client_id);
                        int err_fd = open(err_name,O_RDONLY);
                        int n_read = read(err_fd,err_buff, sizeof err_buff);
                        if (n_read!=14){ // Runtime Error
                            printf("Runtime error");
                            if((numbytes = send(new_fd, err_buff, n_read, 0)) == -1) {
                                perror("server: sendto");
                                exit(1);
                            }
                        }
                        else{ // No Runtime error, Checking for the result
                            char out_name[100];
                            sprintf(out_name,"./dummy/out%d.txt",client_id);
                            int res=compare("answer.txt",out_name);
                            if(res==-1){
                                char buff[MAXBUFLEN];
                                char err_buff[MAXBUFLEN];
                                char err_name[100];
                                sprintf(err_name,"./dummy/error%d.txt",client_id);
                                int err_fd = open(err_name,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                                write(err_fd,"Incorrect Output...\n",(sizeof "Incorrect Output...\n")-1);

                                int file1=open("answer.txt",O_RDONLY);
                                int file2=open(out_name,O_RDONLY);
                                int n_read = read(file2,buff,sizeof buff);
                                write(err_fd,"Your Output = ",(sizeof "Your Output = ")-1);
                                write(err_fd,buff,n_read);
                                n_read = read(file1,buff,sizeof buff);
                                write(err_fd,"\n",1);
                                write(err_fd,"Correct Output = ",(sizeof "Correct Output = ")-1);
                                write(err_fd,buff,n_read);
                                write(err_fd,"\0",1);
                                close(err_fd);
                                err_fd = open(err_name,O_RDONLY);
                                n_read=read(err_fd,buff,sizeof buff);
                                if((numbytes = send(new_fd, buff, n_read, 0)) == -1) {
                                    perror("server: sendto");
                                    exit(1);
                                }
                            }
                            else{
                                //printf("Correct Output...\n");
                                if((numbytes = send(new_fd, "Correct Output...\n", sizeof "Correct Output...\n", 0)) == -1) {
                                    perror("server: sendto");
                                    exit(1);
                                }
                            }

                        }
                    }
                
                }
            }
            printf("Client Clossed connection...\n");
            close(new_fd);
            exit(0);
            }
           
            // else{
            //     close(new_fd); // parent doesn't need this
            //     wait(NULL);
            // }
        
    }
    
    
    

    close(sockfd);
    return 0;
}