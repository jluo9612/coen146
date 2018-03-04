/* Name: Jennifer Luo
   Lab section: Friday
   client.c - FTv1 file transfer client using TCP
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <stdint.h>

int main (int argc, char *argv[]) {
	int i;
	int sockfd = 0, n = 0;
	char buff[10];
	struct sockaddr_in serv_addr;

	// check number of arguments
	if (argc != 5) {
		printf ("Usage: Number of arguments is incorrect (correct 5)");
		return 1;
	}

	// set memory
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(buff, '0', sizeof(buff));
	
	// create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Error: Could not create socket \n");
		return 1;
	}

	// server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	
	// inet_pton converts argv[2] into sin_addr
	if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) < 0) {
		printf("Error: inet_pton error \n");
		return 1;
	} else if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) == 0) {
		printf("Usage: src string not valid \n");
		return 1;
	}

	// connect
	if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
	{
		printf ("Error : Connect Failed \n");
		return 1;
	} 

	// write output file name
	if (write(sockfd, argv[4], strlen(argv[4])+1) < 0) {
		printf("Error: Cannot send dest file to socket\n");
		return 1;
	} 

	// open input file
	FILE* fin = fopen(argv[3], "r");
	if (fin == NULL) {
		printf("Error: Cannot open src file!\n");
		return 1;
	}

	// write src content to buff
	while (!feof(fin)) {
		int n = fread(buff, 1, 10, fin);
		write(sockfd, buff, n); // actualsize of string including null char
	}
	printf ("Finished\n");
	fclose(fin);
	close(sockfd);
	return 0;
	
}
