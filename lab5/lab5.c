/*****************************
 * COEN 146 Lab 5
  Name: Jennifer Luo
   Lab section: Friday
	Link-state routing
 *****************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

/*
0 router id
1 number of nodes
2 table cost file
3 host table file
*/

typedef struct machine {
	char name[50];
	char IP[50];
	int port;
}machine_t;

// struct to pass cmat to receive_info
typedef struct matstr {
  	int m, n;
  	int * mat;
}matstr_t;

pthread_mutex_t myMutex;

void printHelp (int row, int col, int cmat[][col]) {
	int i, j;
	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) printf("%d ", cmat[i][j]);
		printf("\n");
	}
}

// thread 1 func
void* receive_info(void* cmat) {
	// receive array
	int array[3];
	int arrayReceived[3];

	matstr_t *M = (matstr_t*)cmat;
	int m = (*M).m;
	int n = (*M).n;
	int* mat = (*M).mat;
	int sock;
	while ((recvfrom(sock, arrayReceived, sizeof(arrayReceived), 0, NULL, NULL)) < 0) sleep(10);
	
	for (int i = 0 ; i < 100 ; ++i) array[i] = ntohl(arrayReceived[i]);
	pthread_mutex_lock(&myMutex);
	// write to cost table
	*(mat + array[0]*n + array[1]) = array[2];
	*(mat + array[1]*n + array[0]) = array[2];

	pthread_mutex_unlock(&myMutex);
	printf("Cost table changed");
	// printHelp(m, n, mat);
	return NULL;
}

// thread 3 func
void* link_state() {
	// read cost table, compute least costs, output least cost array for current node
	pthread_mutex_lock(&myMutex);

	pthread_mutex_unlock(&myMutex);
	// sleep 10-20 here
	return NULL;
}

// main thread function
main (int argc, char *argv[]) {

	if (argc != 5) {
		printf("Usage: <executable><current router id><number of nodes in graph><cost file name><host file name>\n");
		return 1;
	}
	// initialize cost and hosts matrices
	machine_t myMachines[4];
	int cmat[atoi(argv[2])][atoi(argv[2])];

	// parse files
	FILE* cost = fopen(argv[3], "rw");
	if (cost == NULL) {
		printf("Can't open output file!\n");
		return 1;
	}
	FILE* hosts = fopen(argv[4], "r");
	if (hosts == NULL) {
		printf("Can't open output file!\n");
		return 1;
	}
	int i, j, in = 0;
	for (i = 0; i < atoi(argv[2]); i++) {
		for (j = 0; j < atoi(argv[2]); j++) {
			printf("%d ", in);
			fscanf(cost, "%d", &in);
			cmat[i][j] = in;
		}
	}
	fclose(cost);

	int m, portn = 0;
	char nameb[50];
	char IPb[50];
	while (!feof(hosts) && m < atoi(argv[2])) {
		printf("%s %s %d\n", nameb, IPb, portn);
		fscanf(hosts, "%s %s %d", nameb, IPb, &portn);
		machine_t machine;
		strcpy(machine.name, nameb);
		strcpy(machine.IP, IPb);
		machine.port = portn;
		myMachines[m] = machine;
	}
	fclose(hosts);

	// initialize mat struct
	matstr_t M;
	M.mat = cmat;
	M.m = atoi(argv[2]);
	M.n = atoi(argv[2]);

	// initialize threads
	int rc1, rc3;
	pthread_t thread1, thread3;

	printf("Cost table looks like this initially:\n");
	printHelp(atoi(argv[2]), atoi(argv[2]), cmat);

	int count = 0;
	while (count < 2) {

		if( (rc1 = pthread_create( &thread1, NULL, &receive_info, (void*)(&M)) ) )
		{
			printf("Thread creation failed: %d\n", rc1);
		}

		if( (rc3 = pthread_create( &thread3, NULL, &link_state, NULL)) )
		{
			printf("Thread creation failed: %d\n", rc3);
		} // thread3 started

		// takes keyboard input
		int snode, ncost;
		printf("Enter the destination node(0-3) and a new cost to that node(<node><cost>): ");
		scanf("%d %d", &snode, &ncost);

		// main thread not allowed to update cost table
		if (snode != atoi(argv[1])) {
			// prepare message 
			int msg[3] = {atoi(argv[1]), snode, ncost}; 
			int msgToSend[3];
			for (i = 0; i < 3; ++i) msgToSend[i] = htonl(msg[i]);
			// --------------sending message--------------
			struct sockaddr_in serverAddr; 
			socklen_t addr_size;
			int sock;

			// configure address (1: portnum, 2: server ip address)
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons (myMachines[snode].port); 
			inet_pton (AF_INET, myMachines[snode].IP, &serverAddr.sin_addr.s_addr); // 3rd arg: dest address
			memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
			addr_size = sizeof serverAddr;

			// Create socket
			if ((sock = socket (PF_INET, SOCK_DGRAM, 0)) == -1) printf("Error creating socket\n");

			if ((bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) == -1) printf("Error binding\n");

			printf("Config success, socket created, addr binded\n");

			// send messages
			printf ("Sending...\n");
			while (sendto (sock, &msgToSend, sizeof(msgToSend), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) printf("Sending failed. Please restart\n");
			printf ("Packet sent.\n");
		}
		count++;
		// time buffer for thread 1 to be done
		sleep(10);
		printf("Cost table looks like this now:\n");
		printHelp(atoi(argv[2]), atoi(argv[2]), cmat);
	}	
	pthread_join(thread1, NULL);
	pthread_join(thread3, NULL);
	// main thread waits 30 sec and finish here
	sleep(30);

	return 0;	
}

