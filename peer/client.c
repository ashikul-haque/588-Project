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
#include <time.h>

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

char * getIP(){
	char *ip_port = getConf();
	char *token = strtok(ip_port," ");
	return token;
}

int getPort(){
	char *ip_port = getConf();
	char *token = strtok(ip_port," ");
	token = strtok(NULL," ");
	//puts(token);
	int port = atoi(token);
	return port;
}

typedef struct peerInfo {
	char ip[32];
	char port[20];
	int socket, flag;
	long start, end;
} peerInfo;

int getClientSocket(struct ipPort ip_ports) {
	struct sockaddr_in server;
	int socket_desc;
	
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
	
	puts("Connected");
	
	return socket_desc;
}

void *downloadHandler(void *trackerName){
	peerInfo peerList[100];
	int listSize = 0;
	char* fileName;
	long fileSize;
	char* md5;
	char *trackerFileName = strdup((char *) trackerName);
	int socket_desc;
	
	//getting peer info from tracker file
	FILE *fp = fopen(trackerFileName, "r");
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
	while((read = getline(&line, &len, fp)) != -1) {
		//getting filename and size from first line
		if(i==1) {
			char *token = strtok(line, " ");
			fileName = strdup(token);
			token = strtok(NULL, " ");
			fileSize = atol(token);
			token = strtok(NULL, " ");
			md5 = strdup(token);
			
		}
		else if(i!=1 && i%2==0) {
			char *token = strtok(line, " ");
			//peerList[listSize].ip = token;
			sprintf(peerList[listSize].ip,"%s",token);
			token = strtok(NULL, " ");
			//peerList[listSize].port = token;
			sprintf(peerList[listSize].port,"%s",token);
			
			struct ipPort ip_ports;
			sprintf(ip_ports.ip,"%s",peerList[listSize].ip);
			sprintf(ip_ports.port,"%s",peerList[listSize].port);
			
			int socket = getClientSocket(ip_ports);
			peerList[listSize].socket = socket;
		}
		else if(i!=1 && i%2==1) {
			char *token = strtok(line, " ");
			peerList[listSize].start = atol(token);
			token = strtok(NULL, " ");
			peerList[listSize].end = atol(token);
			listSize++;
		}
		i++;
	}
	
	fclose(fp);
	
	//sort peers according to their start byte
	for(int j=0;j<listSize;j++){
		for(int k=j+1;k<listSize;k++){
			peerInfo temp;
			if(peerList[k].start < peerList[j].start) {
				temp = peerList[j];
				peerList[j] = peerList[k];
				peerList[k] = temp;
			}
		}
	}
	
	//printf("list size %d\n",listSize);
	//printf("%s %s %ld %ld\n", peerList[0].ip, peerList[0].port, peerList[0].start, peerList[0].end);
	
	char my_reply[2000];
	char buffer[SIZE];
	bzero(my_reply, 2000);
	bzero(buffer, SIZE);
	long rcvBytes = 0;
	int currPeer=0;
	FILE *nfp = fopen(fileName,"w");
	int c = 0;
	
	while(rcvBytes <= fileSize) {
		
		long start_t = rcvBytes;
		long end_t = start_t+SIZE;
		if(end_t > fileSize) {
			end_t = fileSize;
		}
		
		struct ipPort ip_ports;
		
		//find ip port suitable for next download
		if(peerList[currPeer].start > start_t) {
			int t = currPeer-1;
			while(t>=0) {
				if(peerList[t].start <= start_t){
					currPeer = t;
					break;
				}
				t--;
			}
		}
		
		if(peerList[currPeer].end < end_t) {
			int t = 0;
			while(t<listSize) {
				if(peerList[t].end >= end_t && peerList[t].start <= start_t){
					currPeer = t;
					break;
				}
				t++;
			}
		}

		/*if(peerList[currPeer].end < end_t){
			end_t = peerList[currPeer].end;
		}*/
		
		//printf("Current peer %d\n",currPeer);
		//printf("%ld %ld %ld\n",start_t,end_t,fileSize);
		
		sprintf(ip_ports.ip,"%s",peerList[currPeer].ip);
		sprintf(ip_ports.port,"%s",peerList[currPeer].port);
		int socket_desc = peerList[currPeer].socket;
		
		sprintf(my_reply,"get %s %ld %ld",fileName, start_t, end_t);
		//puts("Sending get request");
		if(write(socket_desc , my_reply, strlen(my_reply))<0){
			puts("get failed!!");
		}
		
		int n;
		n = recv(socket_desc, buffer, SIZE, 0);
		//printf("Received %d bytes\n",n);
    		if (n <= 0){
    			puts("Retry");
    			bzero(my_reply, 2000);
			bzero(buffer, SIZE);
			rcvBytes = start_t;
      			continue;
    		}
		
		fwrite(buffer, 1, n, nfp);
		//puts("wrote in file\n");
		bzero(my_reply, 2000);
		bzero(buffer, SIZE);
		
		if(end_t == fileSize){
			fclose(nfp);
			
			//check md5
			if(strcmp(calculate_file_md5(fileName),md5) == 0){
				puts("md5 check done. Received Correctly");
			}
			else{
				printf("md5: %s\nreceived md5: %s\n",md5, calculate_file_md5(fileName));
				puts("md5 check done. Not received Correctly");
			}
			
			for(int j=0;j<listSize;j++){
				close(peerList[j].socket);
			}
			printf("Download completed for %s\n",fileName);
			sprintf(ip_ports.ip,"127.0.0.1");
			sprintf(ip_ports.port,"7658");
			
			puts("Updating tracker");
			socket_desc = getClientSocket(ip_ports);
			sprintf(my_reply,"update %s 0 %ld %s",fileName, end_t, getConf());
			if(write(socket_desc , my_reply, strlen(my_reply))<0){
				puts("update failed!!");
			}
			close(socket_desc);
			
			return 0;
		}
		rcvBytes = end_t;
		currPeer++;
		
		if(currPeer == listSize){
			currPeer = 0;
		}
		
		
  		//printf("%d\n",msec);
  		if( c == 2000) {
  			sprintf(ip_ports.ip,"127.0.0.1");
			sprintf(ip_ports.port,"7658");
			
			//puts("Updating tracker");
			socket_desc = getClientSocket(ip_ports);
			sprintf(my_reply,"update %s 0 %ld %s",fileName, end_t, getConf());
			if(write(socket_desc , my_reply, strlen(my_reply))<0){
				puts("update failed!!");
			}
			close(socket_desc);
			
			c=-1;
  		}
  		c++;
	}
}

//function to handle communication with tracker_server and other peers as client
void clientThread(void *ip_port){
	
	struct ipPort ip_ports = *(struct ipPort*)ip_port;
	int socket_desc = getClientSocket(ip_ports);
	//struct sockaddr_in server;
	
	char *message , server_reply[2000], my_reply[2000], input[2000],file_message[2000];
	int read_size;
	
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
			puts("create from client");
			
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
			puts("get from client");
			tempp = strtok(NULL, " ");
			//tempp = strtok(NULL, "\n");
			//tempp = strtok(NULL, "\r");
			
			puts(tempp);
			
			int n;
  			FILE *fp;
  			char *filename = strdup(tempp);
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
  			pthread_t peer_download;
			if(pthread_create(&peer_download, NULL, downloadHandler, filename) < 0) {
    				perror("Could not create server send thread");
    				return;
  			}
  			
  			printf("\n");
  			char end[50];
  			//sprintf(end,"%d bytes received",i);
  			sprintf(end,"%s tracker received",filename);
  			puts(end);
  			//printf("%stest",filename);
  			
  			//reconnect to tracker
  			socket_desc = getClientSocket(ip_ports);
  		}
  		
  		else if(strcmp(tempp, "quit") == 10) {
  			puts("quit from tracker server");
  			close(socket_desc);
			return;
		}
  		
  		bzero(my_reply, 2000);
		bzero(server_reply, 2000);
		bzero(input, 2000);
		bzero(file_message, 2000);
	}
	return;
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
			//puts("get from peer server");
			tempp = strtok(NULL, " ");
			//tempp = strtok(tempp, "\n");
			char *fileName = strdup(tempp);
			tempp = strtok(NULL, " ");
			char *start = strdup(tempp);
			tempp = strtok(NULL, " ");
			char *end = strdup(tempp);
			
			long start_t = atol(start);
			long end_t = atol(end);
			
			FILE *fp;
			//sprintf(tempp, "%s",tempp);
			//puts(tempp);
			
			if( access( fileName, F_OK ) == 0 ) {
				fp = fopen(fileName, "r");
  				char data[SIZE];
  				long i = 0;
  				bzero(data, SIZE);
  				int bs;
  				
  				while(!feof(fp)) {
  					bs = fread(data, 1, sizeof(data), fp);
					if(i==start_t){
						write(sock, data, bs);
						//printf("[LOOP] %d bytes sent\n",bs);
						break;
					}
    					
    					bzero(data, SIZE);
    					i+=SIZE;	
  				}
  				
  				//puts(calculate_file_md5(fileName));
  				//printf("\n");
  				char end[50];
  				//sprintf(end,"%ld bytes sent",i);
  				//puts(end);	
  				fclose(fp);
			} else{
				printf("File not found\n");
				close(sock);	
			}
			bzero(client_message, 2000);
			bzero(my_message, 2000);
			bzero(file_message, 2000);
			free(fileName);
			free(start);
			free(end);
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
	server.sin_addr.s_addr = inet_addr( getIP() ); 
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
			
			//int res = *(int *)clientThread((void *) ip_port);
			
			clientThread((void *) ip_port);
			
			/*pthread_t tracker_server;
			if(pthread_create(&peer_server, NULL, clientThread, (void *) ip_port) < 0) {
    				perror("Could not create server send thread");
    				return -1;
  			}*/
  			//free(ip_port);
			//printf("Disconnected from %s:%s\n",ip,port);
		}
		if(strcmp(tempp, "quit") == 10) {
			exit(0);
		}
		bzero(input, 2000);
		bzero(file_message, 2000);
  	}
	
	
	return 0;
}
