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

struct machine {
	char name[50];
	char IP[50];
	int port;
};

pthread_mutex_t count_mutex;
int count = 0;

// thread 1 func
void* receive_info() {
	// update new cost in cost matrix
	pthread_mutex_lock(&count_mutex);
	count++;
	pthread_mutex_unlock(&count_mutex);
	printf("Count: %d\n", count);
}

// thread 3 func
void* link_state() {
	pthread_mutex_lock(&count_mutex);
	count++;
	pthread_mutex_unlock(&count_mutex);
	printf("Count: %d\n", count);
}

// main thread function
main (int argc, char *argv[]) {

	// initialize cost and hosts matrices
	struct machine myMachines[4];
	int cmat[atoi(argv[2])][atoi(argv[2])];
	
	// count initialization
	int count = 0;

	if (argc != 5) {
		printf("Usage: <executable><current router id><number of nodes in graph><cost file name><host file name>\n");
		return 1;
	}
	// open files
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
	// parse cost and host matrices
	while (!feof(cost) && i < atoi(argv[2])) {
		if (j >= 3) {
			j = 0;
			i++;		
		}
		printf("%d", in);
		fscanf(cost, "%d", &in);
		cmat[i][j++] = in;
	}
	fclose(cost);

	int m, portn = 0;
	char nameb[50];
	char IPb[50];
	while (!feof(hosts) && m < atoi(argv[2])) {
		printf("%s %s %d\n", nameb, IPb, portn);
		fscanf(hosts, "%s %s %d", nameb, IPb, &portn);
		struct machine machine;
		strcpy(machine.name, nameb);
		strcpy(machine.IP, IPb);
		machine.port = portn;
		myMachines[m] = machine;
	}
	fclose(hosts);

	// message to all other machines


	// threads 1 and 3
	int rc1, rc3;
	pthread_t thread1, thread3;
	
	if( rc1 = pthread_create( &thread1, NULL, &receive_info, NULL) )
	{
		printf("Thread creation failed: %d\n", rc1);
	}

	if( rc3 = pthread_create( &thread3, NULL, &link_state, NULL) )
	{
		printf("Thread creation failed: %d\n", rc3);
	}

	pthread_join(thread1, NULL);
	pthread_join(thread3, NULL);

	return 0;	
}
