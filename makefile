all: server client

server: ./tracker/tracker.c
	gcc -pthread -o ./tracker/tracker ./tracker/tracker.c

client: ./peer/client.c
	gcc -pthread -o ./peer/peer ./peer/client.c -lcrypto
