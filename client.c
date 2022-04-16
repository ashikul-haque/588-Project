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

int connected;

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

//function to handle communication with tracker_server and other peers as client
void *clientThread(void *unused){

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
	server.sin_addr.s_addr = inet_addr("192.168.31.37");
	server.sin_family = AF_INET;
	server.sin_port = htons( 7658 );

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
  		
  		bzero(my_reply, 2000);
		bzero(server_reply, 2000);
		bzero(input, 2000);
		bzero(file_message, 2000);
	}
}

int main(int argc , char *argv[])
{
	
	//int id = atoi(argv[1]);
	//printf("\n%d\n",id);
	
	pthread_t client_thread;
	if(pthread_create(&client_thread, NULL, clientThread, NULL) < 0) {
    		perror("Could not create server send thread");
    		return -1;
  	}
  	
  	int connected = 1;
  	
  	while(connected >=0 ){
  	}
	
	
	return 0;
}
