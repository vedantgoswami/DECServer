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
#include "server.h"
#include "thread_struct.h"

int compile_file(int id){
    char err_name[100];
    sprintf(err_name,"./dummy/error%d.txt",id);
    int err_fd=open(err_name,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    write(err_fd,"COMPILATION ERROR...\n",sizeof "COMPILATION ERROR...\n"-1);
    close(err_fd);
    char exec[100];
    sprintf(exec,"gcc -o ./dummy/recved%d ./dummy/recved%d.c 2>> ./dummy/error%d.txt",id,id,id);
    int compileResult = system(exec);
    char executableFileName[100];
	sprintf(executableFileName,"./dummy/recved%d",id);
	int checkifCompiled = open(executableFileName,O_RDONLY);
    if(checkifCompiled==-1){
        close(checkifCompiled);
    	return 1;
    }
    close(checkifCompiled);
    return 0;
}

int execute_file(int id){
    char err_name[100];
    sprintf(err_name,"./dummy/error%d.txt",id);
    int err_fd=open(err_name,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    write(err_fd,"RUNTIME ERROR\n",sizeof "RUNTIME ERROR\n"-1);
    close(err_fd);
    char exec[100];
    sprintf(exec,"./dummy/recved%d 2>> ./dummy/error%d.txt 1> ./dummy/out%d.txt",id,id,id);
    int execResult = system(exec);
    return execResult;
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
    close(file1);
    close(file2);
    return 1;
}



void evaluation(void *arg){
    struct Pair *pair = (struct Pair*)arg;
    int client_id = pair->client_id;
    int new_fd = pair->fd;
    // pid_t tid = gettid();
    // printf("thread: %d %d %d\n",client_id, new_fd,tid);
    // close(new_fd);
    // pthread_exit("done");
    int numbytes;
    char buffer[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_in serv_addr, their_addr;
    while(1){
            numbytes=recv(new_fd,buffer,sizeof buffer,0);
            if(numbytes==5){
                printf("Client #%d Clossed connection...\n",client_id);
                send(new_fd,"Bye",sizeof "Bye", 0);
                close(new_fd);
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
                return ;
            }
            int total = 0;
            char filename[100];
            sprintf(filename,"./dummy/recved%d.c",client_id);
            int c_file_fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            send(new_fd,"OK",sizeof "OK",0);
            printf("Bytes Recvd%d: %d\n",client_id,numbytes);
            int total_file_size = atoi(buffer);
            int remaining = total_file_size-total;
            while(remaining!=0){
                numbytes=recv(new_fd,buffer,sizeof buffer,0);
                if(numbytes==0){
                    perror("recv error");
                    //pthread_exit("Done");
                }
                printf("Bytes Recvd: %d\n",numbytes);
                total+=numbytes;
                remaining=total_file_size-total;
                write(c_file_fd,buffer,numbytes);
                bzero(buffer, sizeof buffer);
            }
            fflush(stdout);
            close(c_file_fd);
            pthread_t wthrd;
            struct WorkerPair *workerpair = (struct WorkerPair *)malloc(sizeof(struct WorkerPair));
            workerpair->sockfd = new_fd;
            workerpair->client_id = client_id;
            int compilationStatus = compile_file(client_id);
    if(compilationStatus==1){ // Compilation Failure
        printf("Compilation Failed!\n");
        char err_buff[256];
        char err_name[100];
        sprintf(err_name,"./dummy/error%d.txt",client_id);
        int err_fd = open(err_name,O_RDONLY);
        int n_read = read(err_fd,err_buff, sizeof err_buff);
        close(err_fd);
        if ((numbytes = send(new_fd, err_buff, n_read,0)) == -1) {
            perror("server: send#");
            //pthread_exit("server: send");
        }
        printf("Number of byte sent: %d\n",numbytes);
        // system("rm -f error.txt");
    }
    else{  //Successfully Compiled, Checking for the runtime
            //printf("Compilation Success...\n");
        fflush(stdout);
        int execResult = execute_file(client_id);
        char err_buff[MAXBUFLEN];
        char err_name[100];
        sprintf(err_name,"./dummy/error%d.txt",client_id);
        int err_fd = open(err_name,O_RDONLY);
        int n_read = read(err_fd,err_buff, sizeof err_buff);
        close(err_fd);
        if (execResult!=0){ // Runtime Error
            printf("Runtime error");
            if((numbytes = send(new_fd, err_buff, n_read, 0)) == -1) {
                perror("server: sendto");
                //pthread_exit("server: sendto");
            }
            printf("Number of byte sent: %d\n",numbytes);
           
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
                // int file1=open("answer.txt",O_RDONLY);
                // int file2=open(out_name,O_RDONLY);
                // int n_read = read(file2,buff,sizeof buff);
                // write(err_fd,"Your Output = ",(sizeof "Your Output = ")-1);
                // write(err_fd,buff,n_read);
                // n_read = read(file1,buff,sizeof buff);
                // write(err_fd,"\n",1);
                // write(err_fd,"Correct Output = ",(sizeof "Correct Output = ")-1);
                // write(err_fd,buff,n_read);
                // close(err_fd);
                err_fd = open(err_name,O_RDONLY);
                n_read=read(err_fd,buff,sizeof buff);
                close(err_fd);
                if((numbytes = send(new_fd, buff, n_read, 0)) == -1) {
                    perror("server: sendto");
                    //pthread_exit("server: sendto");
                }
                printf("Number of byte sent: %d\n",numbytes);
               
            }
            else{
                printf("Correct Output...\n");
                fflush(stdout);
                if((numbytes = send(new_fd, "Correct Output...\n", sizeof "Correct Output...\n", 0)) == -1) {
                    perror("server: sendto");
                    //pthread_exit("server: sendto");
                }
                printf("Number of byte sent: %d\n",numbytes);
                
                }
        }
    }
            bzero(buffer, sizeof buffer);
    }
}
