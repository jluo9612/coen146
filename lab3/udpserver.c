/*****************************
 * COEN 146 Lab 3 
  Name: Jennifer Luo
   Lab section: Friday
	FTv2 UDP RDT 2.2 Server
 *****************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

int die(char *str) 
{
	perror(str);
	return 1;
}

/********************
 * main
 ********************/
int main (int argc, char *argv[])
{
	// typedefs
	typedef struct {
		int ackseq; // 32 bit
		int length; // 32 bit, length of data
	} HEADER;

	typedef struct {
		HEADER header;
		char data[10];
		uint8_t checksum;
	} PACKET;

	
	int sock, nBytes;
	uint8_t cksum = 0;
	char buffer[10];
	char namebuf[1024];
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;

    if (argc != 2) die("need the port number\n");
 
	// init 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) die ("socket error\n");

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0) die("bind error\n");

	printf("Waiting for output file name...\n");
	// receive output name
	if ((recvfrom (sock, namebuf, sizeof(namebuf), 0, (struct sockaddr *)&serverStorage, &addr_size)) > 0) 
	{
		printf("This is output name: %s\n", namebuf);
	}

	FILE* fout = fopen(namebuf, "w");
	if (fout == NULL) {
		printf("Can't open output file!\n");
		return 1;
	}

	while (1)
	{	
		printf("\nServer running. Watching for datagrams\n");
		// receive  datagram
		PACKET cpacket;
		while (recvfrom (sock, &cpacket, sizeof(cpacket), 0, (struct sockaddr *)&serverStorage, &addr_size) == -1) die("Error receiving packet from client\n");
	
		if (cpacket.header.length == 0) {
			printf("Last client packet identified. Closing server socket\n");
			break;
		}
		
		printf("Client packet received.\n");
		// save checksum, set to 0, compute checksum
		uint8_t temp = cpacket.checksum;
		cpacket.checksum = 0;
		// calculate checksum 
		char* it = (char*)& cpacket;
		for (i = 0; i < sizeof(it); ++i) cksum ^= it[i];
		printf("Checksum calculated.\n");
	
		if (cksum == temp) { // if checksum matched, send ACK with ackn
			printf("Checksum matched. Delivering data.\n");
			//deliver data
			printf("Getting: %s\n", cpacket.data);
			strcpy(buffer, cpacket.data);
			fwrite(buffer, 1, cpacket.header.length, fout);
			printf("Data delivered.\n");

			printf("Sending corresponding ACK.\n");
			// make spacket
			HEADER header;
			header.ackseq = cpacket.header.ackseq;
			printf("%d\n", header.ackseq);
			header.length = 0;
			PACKET spacket;
			spacket.header = header;
			strcpy(spacket.data, "");
			spacket.checksum = 0;
			while (sendto (sock, &spacket, sizeof(spacket), 0, (struct sockaddr *)&serverStorage, addr_size) == -1) die ("Sending ACK failed. Retrying\n");
		} else { // else, ackn = 1-seqack
			printf("Checksum matching failed. Sending diff ACK\n");
			// make spacket
			HEADER header;
			header.ackseq = 1-cpacket.header.ackseq;
			printf("%d\n", header.ackseq);
			header.length = 0;
			PACKET spacket;
			spacket.header = header;
			strcpy(spacket.data, "");
			spacket.checksum = 0;
			while (sendto (sock, &spacket, sizeof(spacket), 0, (struct sockaddr *)&serverStorage, addr_size) == -1) die ("Sending ACK failed. Retrying\n");
		}

	}

	fclose(fout);

	return 0;
}
