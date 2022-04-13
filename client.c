#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>

int main(int argc , char *argv[])
{
	int socket_desc;
	struct sockaddr_in server;
	char *message , server_reply[2000], my_reply[2000];
	int read_size;
	int id = atoi(argv[1]);
	printf("\n%d\n",id);
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 7658 );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	
	puts("Connected\n");
	char temp1[100];
	sprintf(temp1,"ashik.pdf,150,awesome,dummy,dummy");
	strcat(my_reply, temp1);
	while( write(socket_desc , my_reply , strlen(my_reply)) > 0 )
	{
		char temp[100];
		scanf("%s", temp);
		bzero(my_reply, 2000);
		strcat(my_reply, temp);
		recv(socket_desc , server_reply , 2000 , 0);
		puts("Server: ");
		puts(server_reply);
	}
	
	//Send some data
	/*char temp[100];
	sprintf(temp,"Hello from client%d",id);
	printf("%s\n",temp);
	strcat(my_reply, temp);
	if( send(socket_desc , my_reply , strlen(my_reply) , 0) < 0)
	{
		puts("Send failed");
		return 1;
	}
	puts("Data Send\n");
	
	//Receive a reply from the server
	if( recv(socket_desc, server_reply , 2000 , 0) < 0)
	{
		puts("recv failed");
	}
	puts("Reply received\n");
	puts(server_reply);*/
	
	return 0;
}
