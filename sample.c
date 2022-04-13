#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write

#include<pthread.h> //for threading , link with lpthread

void *connection_handler(void *);

int main(int argc , char *argv[])
{
	int socket_desc , new_socket , c , *new_sock;
	struct sockaddr_in server , client;
	char *message;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 7658 );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");
		
		//Reply to the client
		//message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
		//write(new_socket , message , strlen(message));
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = new_socket;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("could not create thread");
			return 1;
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");
	}
	
	if (new_socket<0)
	{
		perror("accept failed");
		return 1;
	}
	
	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char client_message[2000], my_message[2000];
	//message = "I am your handler\n";
	//write(sock , message , strlen(message));
	
	//Receive a message from client
	int i=0;
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{
		char file_message[2000];
		strcat(file_message, client_message);
		puts(client_message);
		//getting file name
		char *tempp;
		tempp = strtok(client_message, ",");
		//creating tracker file
		char *token;
		token = strtok(tempp, ".");
		sprintf(token, "%s.track",token);
		FILE *fp;
		fp = fopen(token,"w");
		if ( fp )
   		{
	   		fputs(file_message,fp);
    		}
    		else{
    			printf("failed to open file\n");
    		}
		fclose(fp);
		char temp[100];
		sprintf(temp,"Got it %d!!",i++);
		strcat(my_message,temp);
		write(sock , my_message , strlen(my_message));
		bzero(client_message, 2000);
		bzero(my_message, 2000);
		bzero(file_message, 2000);
	}
	
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}
