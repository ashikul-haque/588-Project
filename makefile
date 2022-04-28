all: server client

server: ./tracker/server.c
	gcc -pthread -o ./tracker/tracker ./tracker/server.c

client: ./peer/client.c
	gcc -pthread -o ./peer/peer ./peer/client.c -lcrypto
