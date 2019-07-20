/*
 ============================================================================
 Name        : main.c
 Author      : GodisC00L
 Description : main for server side in internet radio project
 ============================================================================
 */
#include "serverFunc.h"

int main(int argc, char *argv[]) {
	uint16_t tcpPort, udpPort;
	char* multicastIP;
	stationInfo *temp, *endOfList;
	int size, i;

	/* Check if we have all necessary arguments are there*/
	if (argc < 5) {
		errno = EIO;
		errorFunc("Usage: TCP Port, Multicast IP, UDP Port, song location");
	}

	/* read arguments from terminal */
	size = strlen(argv[2]);
	if(!(multicastIP = (char *)calloc(size + 1,sizeof(char))))
		terminate(failedToMallocStr);
	memcpy(multicastIP, argv[2], size);
	udpPort = (uint16_t) atoi(argv[3]);
	tcpPort = (uint16_t) atoi(argv[1]);

	/* creates radio station for each song */
	size = argc - 4;
	for(i = 0; i < size; i++){
		if (!(temp = calloc(1, sizeof(stationInfo)))){
			free(multicastIP);
			terminate(failedToMallocStation);
		}
		addStation(temp, argv[i+4], multicastIP, udpPort);
		if(!stationList){
			stationList = temp;
			endOfList = stationList;
		}
		else{
			endOfList->next = temp;
			endOfList = endOfList->next;
		}
		pthread_create(&(temp->station), NULL, &playStation, (void *)&temp->stationNum);
	}
	free(multicastIP);
	/* Establish TCP welcome socket */
	handleTcpConnections(tcpPort);
	return 0;
}

