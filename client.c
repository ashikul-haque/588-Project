#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include<pthread.h>

#define SIZE 1024

struct ipPort{
	char ip[30];
	char port[20];
};

off_t fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

char * calculate_file_md5(const char *filename) {
	unsigned char c[MD5_DIGEST_LENGTH];
	int i;
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];
	char *filemd5 = (char*) malloc(33 *sizeof(char));

	FILE *inFile = fopen (filename, "rb");
	if (inFile == NULL) {
		perror(filename);
		return 0;
	}

	MD5_Init (&mdContext);

	while ((bytes = fread (data, 1, 1024, inFile)) != 0)

	MD5_Update (&mdContext, data, bytes);

	MD5_Final (c,&mdContext);

	for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
		sprintf(&filemd5[i*2], "%02x", (unsigned int)c[i]);
	}

	//printf("calculated md5:%s ", filemd5);
	//printf (" %s\n", filename);
	fclose (inFile);
	return filemd5;
}

char * getConf(){
	FILE * fp;
	fp = fopen("client.config", "r");
	char *conf;
	char *result = (char*) malloc(50 *sizeof(char));
    	size_t len = 0;
    	ssize_t read;
	read = getline(&conf, &len, fp);
	//conf = strtok(conf,"\n");
	//conf = strtok(NULL,"\n");
	sprintf(result,"%s",conf);
	//printf("%s test",conf);
	return result;
}

int getPort(){
	char *ip_port = getConf();
	char *token = strtok(ip_port," ");
	token = strtok(NULL," ");
	//puts(token);
	int port = atoi(token);
	return port;
}

//function to handle communication with tracker_server and other peers as client
void *clientThread(void *ip_port){
	
	struct ipPort ip_ports = *(struct ipPort*)ip_port;
	int socket_desc;
	struct sockaddr_in server;
	char *message , server_reply[2000], my_reply[2000], input[2000],file_message[2000];
	int read_size;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	//server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_addr.s_addr = inet_addr(ip_ports.ip);
	server.sin_family = AF_INET;
	server.sin_port = htons( atoi(ip_ports.port) );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		exit(0);
	}
	
	puts("Connected\n");
	
	bzero(my_reply, 2000);
	bzero(server_reply, 2000);
	bzero(input, 2000);
	bzero(file_message, 2000);
	while(fgets(input, 2000, stdin) != NULL) {
		strcat(file_message, input);
		char *tempp;
		tempp = strtok(file_message, " ");
		//puts(tempp);
		if(strcmp(tempp, "create") == 0) {
			//puts("create from client");
			
			//get file name
			tempp = strtok(NULL, " ");
			char fileName[200];
			sprintf(fileName,"%s", tempp);
			
			//give description as description
			tempp = strtok(NULL, " ");
			tempp = strtok(tempp,"\n");
			char fileDesc[200];
			sprintf(fileDesc,"%s", tempp);
			
			//final message
			sprintf(my_reply,"create %s %ld %s %s %s",fileName, (long int) fsize(fileName), fileDesc, calculate_file_md5(fileName),getConf());
			//puts(my_reply);
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , 2000 , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "update") ==0){
			if(write(socket_desc, input , strlen(input))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , 2000 , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "req") ==0){
			if(write(socket_desc , input , strlen(input))<0){
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
  			char *filename = "./download/dummy.txt";
  			char buffer[SIZE];
  			fp = fopen(filename, "w");
			int i=0;
			if(write(socket_desc , input , strlen(input))<0){
				puts("get failed!!");
			}
			bzero(buffer, SIZE);
  			while (1) {
  				//puts("in file receive loop");
    				n = read(socket_desc, buffer, SIZE);
    				if (n <= 0){
      					break;
    				}
    				fwrite(buffer, 1, n, fp);
    				//fprintf(fp, "%s", buffer);
    				bzero(buffer, SIZE);
    				i++;
    				//printf("%d->",++i);
  			}
  			fclose(fp);
  			//after getting the tracker file from server
  			//check tracker file and send download request to peers
  			//asking for chunks
  			printf("\n");
  			char end[50];
  			sprintf(end,"%d bytes received",i);
  			puts(end);
  		}
  		
  		else if(strcmp(tempp, "quit") == 10) {
			return 0;
		}
  		
  		bzero(my_reply, 2000);
		bzero(server_reply, 2000);
		bzero(input, 2000);
		bzero(file_message, 2000);
	}
	return 0;
}

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
	char file_message[2000];
	bzero(client_message, 2000);
	bzero(my_message, 2000);
	bzero(file_message, 2000);
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{
		strcat(file_message, client_message);
		char *tempp;
		tempp = strtok(file_message, " ");
		if(strcmp(tempp, "get") ==0) {
			puts("get from server");
			tempp = strtok(NULL, " ");
			tempp = strtok(tempp, "\n");
			FILE *fp;
			sprintf(tempp, "%s",tempp);
			//puts(tempp);
			
			if( access( tempp, F_OK ) == 0 ) {
				fp = fopen(tempp, "r");
				struct stat s;
				//strcat(my_message,s.st_size);
				//write(sock , my_message , strlen(my_message));
				//printf("File size %ld sent!!\n",s.st_size);
  				char data[SIZE];
  				int i = 0;
  				bzero(data, SIZE);
  				int bs = fread(data, 1, sizeof(data), fp);
  				while(!feof(fp)) {
  					i=i+1;
    					write(sock, data, bs);
    					//bzero(data, SIZE);
    					bs = fread(data, 1, sizeof(data), fp);
    					//printf("%d->",i);	
  				}
  				printf("\n");
  				char end[50];
  				sprintf(end,"%d bytes sent",i);
  				puts(end);
  				close(sock);	
			} else{
				printf("failed to open file\n");
				close(sock);	
			}
		}
		
	}
	
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("Connection terminated");
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}

void *peerServer(void *unused){
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
	server.sin_port = htons( getPort() );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		exit(1);
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
			exit(1);
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");
	}
	
	if (new_socket<0)
	{
		perror("accept failed");
		exit(1);
	}
	exit(0);
	
} 

int main(int argc , char *argv[])
{
	
	//int id = atoi(argv[1]);
	//printf("\n%d\n",id);
	
	pthread_t peer_server;
	if(pthread_create(&peer_server, NULL, peerServer, NULL) < 0) {
    		perror("Could not create server send thread");
    		return -1;
  	}
  	
  	//int connected = 1;
  	
  	char input[2000], file_message[2000];
  	
  	bzero(input, 2000);
	bzero(file_message, 2000);
  	while(fgets(input, 2000, stdin) != NULL){
  		strcat(file_message, input);
		char *tempp;
		tempp = strtok(file_message, " ");
		//puts(tempp);
		if(strcmp(tempp, "connect") == 0) {
			struct ipPort *ip_port = malloc(sizeof(struct ipPort));
			//get ip
			tempp = strtok(NULL, " ");
			sprintf(ip_port->ip,"%s", tempp);
			
			//get port
			tempp = strtok(NULL, " ");
			sprintf(ip_port->port,"%s", tempp);
			
			pthread_t tracker_server;
			if(pthread_create(&peer_server, NULL, clientThread, (void *) ip_port) < 0) {
    				perror("Could not create server send thread");
    				return -1;
  			}
  			//free(ip_port);
			//printf("Disconnected from %s:%s\n",ip,port);
		}
		if(strcmp(tempp, "quit") == 10) {
			exit(0);
		}
  	}
	
	
	return 0;
}
