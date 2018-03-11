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
0 executable
1 router id
2 number of nodes
3 table cost file
4 host table file
*/

#define ROW 4
#define COL 4

int cmat[ROW][COL];
int count = 0;

typedef struct machine {
	char name[50];
	char IP[50];
	int port;
}machine_t;

pthread_mutex_t myMutex;

void printHelp (int row, int col, int cmat[][col]) {
	int i, j;
	for (i = 0; i < row; i++) {
		for (j = 0; j < col; j++) printf("%d ", cmat[i][j]);
		printf("\n");
	}
}

// thread 1 func
void* receive_info() {
	// receive array
	int array[3];
	int arrayReceived[3];
	int sock;
	while ((recvfrom(sock, arrayReceived, sizeof(arrayReceived), 0, NULL, NULL)) <= 0) sleep(10); 
	for (int i = 0 ; i < 100 ; ++i) array[i] = ntohl(arrayReceived[i]);

	pthread_mutex_lock(&myMutex);
	// write to cost table
	cmat[array[0]][array[1]] = 11;
	cmat[array[1]][array[0]] = 11;
	count += 1;
	pthread_mutex_unlock(&myMutex);

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
			printf("%d ", in);
			fscanf(cost, "%d", &in);
			cmat[i][j] = in;
		}
	}
	fclose(cost);

	// not parsed correctly
	int m, portn = 0;
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

	printf("Cost table looks like this initially:\n");
	printHelp(ROW, COL, cmat);

	// initialize threads
	int rc1, rc3;
	pthread_t thread1, thread3;

	// start threads
	if( (rc1 = pthread_create( &thread1, NULL, &receive_info, (void*)cmat) ) )
	{
		printf("Thread creation failed: %d\n", rc1);
	}

	// if( (rc3 = pthread_create( &thread3, NULL, &link_state, NULL)) )
	// {
	// 	printf("Thread creation failed: %d\n", rc3);
	// } 

	int ma;
	for (ma = 0; ma < 4; ma++) {
		printf("Machine: %d %s %s %d\n", ma, myMachines[ma].name, myMachines[ma].IP, myMachines[ma].port);
	}

	int ct = 0;
	while (ct < 2) {

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

			struct sockaddr_in serverAddr; 
			socklen_t addr_size;
			int sock;

			// configure address (1: portnum, 2: server ip address)
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons (myMachines[snode].port); 
			inet_pton (AF_INET, myMachines[snode].IP, &serverAddr.sin_addr.s_addr); // 3rd arg: dest address
			memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
			addr_size = sizeof serverAddr;

			// Create socket and binding
			if ((sock = socket (PF_INET, SOCK_DGRAM, 0)) == -1) printf("Error creating socket\n");
			if ((bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) == -1) printf("Error binding\n");
			printf("Config success, socket created, addr binded\n");

			// send messages
			printf ("Sending...\n");
			// server address is wrong?
			while (sendto (sock, &msgToSend, sizeof(msgToSend), 0, (struct sockaddr *)&serverAddr, addr_size) == -1) printf("Sending failed. Please restart\n");
			printf ("Packet sent.\n");
		}
		ct++;
		// time buffer for thread 1 to be done
		sleep(10);
		// should received at this point?
		printf("Count : %d\n", count);
		printf("Cost table looks like this now:\n");
		printHelp(atoi(argv[2]), atoi(argv[2]), cmat);
	}	
	pthread_join(thread1, NULL);
	pthread_join(thread3, NULL);

	// main thread waits 30 sec and finish here
	sleep(2);

	return 0;	
}

