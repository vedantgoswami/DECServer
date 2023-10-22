#ifndef STRUCT_H
#define STRUCT_H
#define MAXQUEUESIZE 10000
// Function declarations
struct Pair{
    int fd;
    int client_id;
    int file_size;
};

struct WorkerPair{
    int sockfd;
    int client_id;
};


#endif




