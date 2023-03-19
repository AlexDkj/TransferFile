all: server client
server: main.o threadpool.o
	gcc main.o threadpool.o -o dataServer -lpthread
client: remoteclient.o
	gcc remoteclient.o -o remoteClient
main.o: main.c
	gcc -c main.c
remoteclient.o: remoteclient.c
	gcc -c remoteclient.c
threadpool.o: threadpool.c
	gcc -c threadpool.c
clean:
	rm *.o
	rm dataServer
	rm remoteClient
