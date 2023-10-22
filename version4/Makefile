n_client=100
n_loop=1
n_sleep=1

all: client server

client: loadtestclient.c test2.c
		gcc -o client loadtestclient.c

server: gradingserver.c server.o 
		gcc -o server2 gradingserver.c server.o

server.o: server.c server.h thread_struct.h
		gcc -c server.c

genplot: avgresponsevsM.txt plot.gnuplot plot2.gnuplot
		 gnuplot plot.gnuplot
		 gnuplot plot2.gnuplot

run_server: server
		./server

run_client: server client loadtest.sh
		./loadtest.sh $(n_client) $(n_loop) $(n_sleep)

clean:
		rm -f client server
		rm -f ./dummy/*
		rm -f *.o