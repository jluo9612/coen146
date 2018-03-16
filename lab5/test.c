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
#include <stdbool.h>

/*
0 executable
1 router id
2 number of nodes
3 table cost file
4 host table file
*/

#define ROW 4
#define COL 4

typedef struct machine {
	char name[50];
	char IP[50];
	int port;
} machine_t;

// initialize cost and hosts matrices
machine_t myMachines[4];
int cmat[ROW][COL];

// thread stuff
pthread_mutex_t myMutex;

void printHelp (int row, int col, int cmat[][col]) {
	int i, j;
	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) printf("%d ", cmat[i][j]);
		printf("\n");
	}
}

// thread 1 fu
void* receive_info(void* ind) {
	int sock;
	// init server
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;

	int myind = (uintptr_t) ind;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ( (short) myMachines[myind].port );
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	if ( (sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) printf ("socket error\n");
	if ( (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr))) != 0) perror("bind error\n");

	while (1) {
		// receive array
		int array[3];
		int arrayReceived[3];
		while ( recvfrom(sock, arrayReceived, sizeof(arrayReceived), 0, (struct sockaddr *)&serverStorage, &addr_size) == 0 ) {
			usleep(100);
		}
		
		int i;
		for (i = 0 ; i < 3 ; ++i) {
			array[i] = ntohl(arrayReceived[i]);
		}
		printf ( "Received cost: %d\n", array[2] );

		pthread_mutex_lock(&myMutex);
		// write to cost table
		cmat[array[0]][array[1]] = array[2];
		cmat[array[1]][array[0]] = array[2];
		printf("Cost table looks like this now:\n");
		printHelp(ROW, COL, cmat);
		pthread_mutex_unlock(&myMutex);
		sleep(1);
	}
}

// thread 3 func
void* link_state(void* ind) {
	int myind = (uintptr_t) ind; 
	bool visited[4];
	int lc[4];
	srand(time(NULL));

	while (1) {
		pthread_mutex_lock(&myMutex);
		int i;
		int min = 0; 
		// initiating least cost array and visited array
		for (i = 0; i < 4; i++) { 
			lc[i] = cmat[myind][i];
			// printf( "Initial lc: %d\n", lc[i] );
			visited[i] = false;
			if (i != myind) {
				min = lc[i]; // min is initiated to distance to one of other nodes
			}
		}
		visited[myind] = true; 
		int closest = 0;

		// finding closest node
		for (i = 0; i < 4; i++) { 
			if (lc[i] != 0 && lc[i] <= min) { // if i != self and lc[i] < min)
				min = lc[i];
				closest = i;
			}
		}
		// printf ( "min: %d\n", min );

		// printf("%d is closest to %d with dist: %d\n", myind, closest, min);
		visited[closest] = true; // closest direct node visited

		// finding least cost path
		int j;
		for (j = 0; j < 4; j++) {
			if (!visited[j]) {
				if (lc[closest] + cmat[closest][j] < lc[j]) lc[j] = lc[closest] + cmat[closest][j];
				visited[j] = true;
			}
		}

		pthread_mutex_unlock(&myMutex);
		// print least costs
		for (i = 0; i < 4; i++) printf( "Least cost from %d to %d is: %d now \n", myind, i, lc[i]);
		printf ( "One cycle completes. \n\n" );

		// sleep 10-20 here
		int s = rand () % 11 + 10;
		sleep( s );
	}

}

// main thread function
main (int argc, char *argv[]) {

	if (argc != 5) {
		printf("Usage: <executable><current router id><number of nodes in graph><cost file name><host file name>\n");
		return 1;
	}

	// parse files
	FILE* cost = fopen(argv[3], "rw");
	if (cost == NULL) {
		printf("Can't open cost file!\n");
		return 1;
	}
	FILE* hosts = fopen(argv[4], "r");
	if (hosts == NULL) {
		printf("Can't open hosts file!\n");
		return 1;
	}
	int i, j, in = 0;
	for (i = 0; i < atoi(argv[2]); i++) {
		for (j = 0; j < atoi(argv[2]); j++) {
			// printf("%d ", in);
			fscanf(cost, "%d", &in);
			cmat[i][j] = in;
		}
	}
	fclose(cost);

	int m = 0;
	int portn = 0;
	char nameb[50];
	char IPb[50];
	while (!feof(hosts) && m < atoi(argv[2])) {
		fscanf(hosts, "%s %s %d", nameb, IPb, &portn);
		machine_t machine;
		strcpy(machine.name, nameb);
		strcpy(machine.IP, IPb);
		machine.port = portn;
		myMachines[m] = machine;
		m++;
	}
	fclose(hosts);

	// initialize threads
	int rc1, rc3;
	pthread_t thread1, thread3;

	// start threads
	if( (rc1 = pthread_create( &thread1, NULL, &receive_info, (void*) (uintptr_t) atoi(argv[1]) ) ) )
	{
		printf("Thread creation failed: %d\n", rc1);
	}

	if( (rc3 = pthread_create( &thread3, NULL, &link_state, (void*) (uintptr_t) atoi(argv[1]) )) )
	{
		printf("Thread creation failed: %d\n", rc3);
	} 

	// printf("Cost table looks like this initially:\n");
	// printHelp(ROW, COL, cmat);

	int ct = 0;
	int sock;
	if ((sock = socket (PF_INET, SOCK_DGRAM, 0)) == -1) printf("Error creating socket\n");

	// int ma;
	// for (ma = 0; ma < 4; ma++) printf( "Machine %s: %s %d\n", myMachines[ma].name, myMachines[ma].IP, myMachines[ma].port );

	while (ct < 2) {
		ct++;
		// takes keyboard input
		int snode = 0;
		int ncost = 0;
		int i;
		printf("Enter a new cost to a node(<node><cost>):\n ");
		scanf("%d %d", &snode, &ncost);

		while (snode > 3) {
			printf("Destination node doesn't exist! Please reenter node: \n");
			scanf("%d %d", &snode, &ncost);
		} 

		while (snode == atoi(argv[1])) {
			printf("Please enter a node other than myself! : \n");
			scanf("%d %d", &snode, &ncost);
		}

		// broadcast to all nodes's receive thread
		for (i = 0; i < 4; i++) {
			// prepare message 
			int msg[3] = {atoi(argv[1]), snode, ncost}; 
			int msgToSend[3];
			int j;
			for (j = 0; j < 3; ++j) msgToSend[j] = htonl(msg[j]);

			// Config for destination
			struct sockaddr_in serverAddr; 
			socklen_t addr_size;
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons (myMachines[i].port); 
			printf ( "Sending to port: %d\n", myMachines[i].port );
			inet_pton (AF_INET, myMachines[i].IP, &serverAddr.sin_addr.s_addr); // 3rd arg: dest address
			memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
			addr_size = sizeof(serverAddr);
			printf("Receiver config complete. Ready to send message. \n");

			// send messages
			printf ("Sending...\n");
			for (j = 0; j < 3; j++) printf ( "Sending %d\n", msg[j]);
			if (sendto (sock, &msgToSend, sizeof(msgToSend), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) printf("Sending failed. Restarting\n");
			printf ("Packet sent.\n");
		}
		printf( "Count: %d\n", ct );
		printf("Cost table looks like this now:\n");
		printHelp(ROW, COL, cmat);
	}	

	// main thread waits 30 sec and finish here
	sleep(2);
	// pthread_exit(NULL);
	// pthread_join(thread1, NULL);
	// pthread_join(thread3, NULL);

	return 0;	
}

