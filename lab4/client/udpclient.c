/*****************************
 * COEN 146 Lab 4
  Name: Jennifer Luo
   Lab section: Friday
	FTv2 UDP RDT 3.0 Client
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
#include <fcntl.h>

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

	typedef struct {
		int tv_sec;
		int tv_usec;
	} timeval;
	
	/* ---------start ----------*/ 

	int sock, i; // socket
	srand(time(NULL)); //seed
	struct sockaddr_in serverAddr; // server address
	socklen_t addr_size;

	// check argument number
	if (argc != 5) die("Incorrect argument number\n");

	// setup for select	
	fd_set readfds;
	fcntl (sock, F_SETFL, O_NONBLOCK);

	// setup timer
	struct timeval tv;
	int rv = 0;

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
	int count =  0; // counter of retransmission of same packet
	// taking stuff from file

	while (!feof(fin) && count < 4)
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
    
		// send packet
		printf ("Sending...\n");
		while (sendto (sock, &packet, sizeof(packet), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Sending failed. Please restart\n");
		printf ("Packet sent.\n");

		// call select
		FD_ZERO (&readfds);
		FD_SET (sock, &readfds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		rv = select(sock+1, &readfds, NULL, NULL, &tv);
		printf("%d\n", rv);
		if (rv < 0) { // select error
			die("Select error\n");
			rv = select(sock+1, &readfds, NULL, NULL, &tv);
		} 

		if (rv == 0) { // Nothing received on server. Timeout.
			printf("Nothing received on server side. Timeout.\n");	
			printf("\nCurrent count is %d.\n", count);			
			while (sendto (sock, &packet, sizeof(packet), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Resending failed. Restarting\n");
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			count++;
		} else if (rv == 1) { // theres data on server
			// get ACK
			PACKET rcvpkt;
			if ((recvfrom(sock, &rcvpkt, sizeof(rcvpkt), 0, NULL, NULL)) > 0) {
				if (rcvpkt.header.ackseq == !seq) { // if NAK
					printf("NAK received. Retransmitting...\n");
					while (sendto (sock, &packet, sizeof(packet), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Retransmission failed. Retrying...\n");
					printf("Retransmission successful.\n");
				} else { // all is well. proceed!
					printf("Correct ACK received. Continuing to send next packet...\n");
					seq = 1-seq;
				}
			} 
		}

	} // end of one sending/receiving cycle


	printf("\n\nCount is: %d\n", count);
	fclose(fin);
	count = 0;

	// send no data packet to finish
	HEADER header;
	header.ackseq = 0;
	header.length = 0;
	PACKET fpacket;
	fpacket.header = header;
	strcpy(fpacket.data, "");
	fpacket.checksum = 0;
	printf ("Sending final packet...\n");
	while (sendto (sock, &fpacket, sizeof(fpacket), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Sending final failed.\n");
	printf ("Final packet sent.\n");

	PACKET frcvpkt;
	while (((recvfrom(sock, &frcvpkt, sizeof(frcvpkt), 0, NULL, NULL)) > 0) && (frcvpkt.header.ackseq == !seq) && count < 4) { // received NAK and count < 4
		rv = select(sock+1, &readfds, NULL, NULL, &tv);
		if (rv == 0) { // timeout, no ACK
			while (sendto (sock, &fpacket, sizeof(fpacket), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Resending failed. Restarting\n");
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			count++;
		} else if (rv == 1) {
			// got ACK, checking ACK number
			printf ("Wrong ACK received. Retransmitting...\n");
			while (sendto (sock, &fpacket, sizeof(fpacket), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) die("Retransmission failed. Retrying...\n");
			printf("Retransmission successful.\n");
		}
	}
	

	return 0;
}
	
