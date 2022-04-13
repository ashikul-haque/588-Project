#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <openssl/md5.h>

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

	printf("calculated md5:%s ", filemd5);
	printf (" %s\n", filename);
	fclose (inFile);
	return filemd5;
}

int main(int argc, char** argv) {
    char* file_name;
    file_name = "1.pdf";
    /*long int file_size;
    
    file_size = (long int) fsize(file_name);
    printf("%ld\n",file_size);*/
    //gcc -o file_size file_size_test.c -lcrypto
    char *new_md5 = calculate_file_md5(file_name);
}
