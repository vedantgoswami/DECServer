#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_BUFF 4096
int main()
{ 	//int num = rand() % 10000 + 1; 
 	for(int i=1;i<=10;i++){
		printf("%d ",i);
	}
	return 0;

}

