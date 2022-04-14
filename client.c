#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>
#define SIZE 1024

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
		
	//server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_addr.s_addr = inet_addr("192.168.31.37");
	server.sin_family = AF_INET;
	server.sin_port = htons( 7658 );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	
	puts("Connected\n");
	//char temp1[100];
	//sprintf(temp1,"ashik.pdf 150 awesome dummy dummy");
	//strcat(my_reply, temp1);
	/*while( write(socket_desc , my_reply , strlen(my_reply)) > 0 )
	{
		char temp[100];
		scanf("%s", temp);
		bzero(my_reply, 2000);
		strcat(my_reply, temp);
		recv(socket_desc , server_reply , 2000 , 0);
		puts("Server: ");
		puts(server_reply);
	}*/
	char file_message[2000];
	while(fgets(my_reply, 2000, stdin) != NULL) {
		strcat(file_message, my_reply);
		char *tempp;
		tempp = strtok(file_message, " ");
		//puts(tempp);
		if(strcmp(tempp, "create") == 0) {
			//puts("create from client");
			//get file size using file name
			//give description as description
			//give md5 from function
			//ip and port from config file
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , 2000 , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "update") ==0){
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , 2000 , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "req") ==0){
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , 2000 , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "get") ==0){
			//puts("get from client");
			//tempp = strtok(NULL, " ");
			int n;
  			FILE *fp;
  			char *filename = "./download/2.txt";
  			char buffer[SIZE];
  			fp = fopen(filename, "w");
			int i=0;
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("get failed!!");
			}
  			while (1) {
  				//puts("in file receive loop");
    				n = recv(socket_desc, buffer, SIZE, 0);
    				if (n <= 0){
      					break;
    				}
    				fprintf(fp, "%s", buffer);
    				bzero(buffer, SIZE);
    				printf("%d->",++i);
  			}
  			fclose(fp);
  			//after getting the tracker file from server
  			//check tracker file and send download request to peers
  			//asking for chunks
  			printf("\n");
  			puts("download complete");
  		}
  		
  		bzero(my_reply, 2000);
		bzero(server_reply, 2000);
		bzero(file_message, 2000);
	}
	
	return 0;
}
