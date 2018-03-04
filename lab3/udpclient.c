/*****************************
 * COEN 146 Lab 3 
  Name: Jennifer Luo
   Lab section: Friday
	FTv2 UDP RDT 2.2 Client
 *****************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

/***********
 *  main
 ***********/

int die(char *str) {
	perror(str);
	return 1;
}

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
	
	/* ---------start ----------*/ 

	int sock, i; // socket
	srand(time(NULL)); //seed
	struct sockaddr_in serverAddr; // server address
	socklen_t addr_size;

	// check argument number
	if (argc != 5) die("Incorrect argument number\n");

	// configure address (1: portnum, 2: server ip address)
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1])); // 2nd arg: port
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr); // 3rd arg: address
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	// Create socket
	if ((sock = socket (PF_INET, SOCK_DGRAM, 0)) == -1) die("Error creating socket\n");
	
	// open file
	FILE* fin = fopen(argv[3], "r");
	if (fin == NULL) {
		printf("Can't open input file!\n");
		return 1;
	} 

	// send output name
	if ((sendto (sock, argv[4], strlen(argv[4])+1, 0, (struct sockaddr *)&serverAddr, addr_size)) > 0) {
		printf("Name of output file sent!\n");
	}


	char buff[10]; // buff to hold data of each packet temporarily
	uint8_t cksum = 0; // checksum
	int seq = 0; // sequence number

	// taking stuff from file
	while (!feof(fin))
	{		
		int n = fread(buff, 1, sizeof(buff), fin);
		printf("I'm getting: %s\n", buff);
		
		// make packet
		HEADER header;
		header.ackseq = seq;
		header.length = n;
		PACKET packet;
		packet.header = header;
		packet.checksum = 0;
		strcpy(packet.data, buff);

		// calculate checksum 
		char* it = (char*)& packet;
		for (i = 0; i < sizeof(it); ++i) cksum ^= it[i];

		// randomize checksum value (checksum or 00000000)
		if (rand() % 2 == 0) cksum = 0;
		packet.checksum = cksum; // how to convert?
		printf("Checksum calculated: %d\n", cksum);
    
		// send initial packet
		printf ("Sending...\n");
		while (sendto (sock, &packet, sizeof(packet), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Sending initial failed. Please restart\n");
		printf ("Packet sent.\n");

		// receiving ACKs
		PACKET rcvpkt;	// pointer for ACK packet from receiver	
		while (((recvfrom(sock, &rcvpkt, sizeof(rcvpkt), 0, NULL, NULL)) > 0) && (rcvpkt.header.ackseq == !seq)) {
			printf ("Wrong ACK received. Retransmitting...\n");
			while (sendto (sock, &packet, sizeof(packet), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Retransmission failed. Retrying...\n");
			printf("Retransmission successful.\n");
		}

		printf("Correct ACK received. Continuing to send next packet...\n");
		seq = 1-seq;
	
	} // end of one sending/receiving cycle

	fclose(fin);

	// send no data packet to finish
	HEADER header;
	header.ackseq = 0;
	header.length = 0;
	PACKET fpacket;
	fpacket.header = header;
	strcpy(fpacket.data, "");
	fpacket.checksum = 0;
	printf ("Sending final packet...\n");
	if (sendto (sock, &fpacket, sizeof(fpacket), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Sending final failed\n");

	return 0;
}
	
