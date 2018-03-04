/* Name: Jennifer Luo
   Lab section: Friday
   server.c - FTv1 file transfer server using TCP
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

int main (int argc, char *argv[])
{
	int n;
	char *p;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	char outputn[1024]; // string for output file name
	char buff[5]; // buff for content

	if (argc != 2) {
		printf ("Usage: Number of arguments is incorrect (correct 2)");
		return 1;
	}

	printf("\n No of cmd line arguments passed = [%d]\n",argc);

	 int count = 0;
	 while (count < argc)
	 {
		 printf("\n arg [%d] = [%s]\n",count+1,argv[count]);
		 count++;
	 }

	// set up server address
	memset (&serv_addr, '0', sizeof (serv_addr));
	memset (buff, '0', sizeof (buff));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons (atoi(argv[1]));

	// create socket, bind socket to server, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0);
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr));
	listen (listenfd, 10);

	// server on
	while (1) {
		// console message
		printf("Server running, listening for connection requests...\n");
		// returns file descriptor for accepted socket (listenfd)
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
		
		// receive output file name and open
		n = read (connfd, outputn, sizeof(outputn));
		if (n < 0) printf("Error reading output file name!\n");
		printf("This is the output name: %s\n", outputn);

		FILE* fout = fopen(outputn, "w");
		if (fout == NULL) {
			printf("Can't open output file \n");
			return 1;
		}

		// receive content
		while ((n = read (connfd, buff, sizeof(buff))) > 0) {
			fwrite(buff, 1, n, fout);
			printf("I'm writing \n");
		} 
		fclose(fout);
		close(connfd);

	}

	return 0;

}
