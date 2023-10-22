#ifndef SERVER_H
#define SERVER_H
#define MAXBUFLEN 2048
int compile_file(int id);
int execute_file(int id);
int compare(char *f1, char f2[100]);
void evaluation(void *arg);
void *handle_client_request(void *arg);

#endif




