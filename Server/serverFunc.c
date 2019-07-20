/*
 ============================================================================
 Name        : serverFunc.c
 Author      : GodisC00L
 Description : functions for server side in internet radio project
 ============================================================================
 */
#include "serverFunc.h"

stationInfo *stationList = NULL;
tcpConnectionInfo *clientList = NULL;

void errorFunc(char *msg) {
    perror(msg);
    exit(errno);
}
/* Add new station to radio */
void addStation(stationInfo *station, const char* songName, const char* mcastIP, uint16_t udpPort){
	const char ttl = TTL;
	/* write song name */
	if (!(station->songName = calloc(strlen(songName) + 1, sizeof(char))))
		terminate(failedToMallocStr);
	memcpy(station->songName, songName, strlen(songName));
	station->stationNum = numOfStation;

	if((station->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		free(station->songName);
		free(station);
		terminate(failedSocket);
	}

	/* set mcast addr */
	bzero((char *)&(station->mcastIp), sizeof(station->mcastIp));
	station->mcastIp.sin_family = AF_INET;
	station->mcastIp.sin_port = htons(udpPort);
	station->mcastIp.sin_addr.s_addr = ntohl(htonl(inet_addr(mcastIP)) + numOfStation);

	/* bind */
	if (bind(station->socket, (struct sockaddr *)&(station->mcastIp), sizeof(station->mcastIp)) < 0){
		free(station->songName);
		free(station);
		terminate(failedBind);
	}

	/* set socket option */
	if (setsockopt(station->socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0){
		free(station->songName);
		free(station);
		terminate(failedSetSockOpt);
	}

	if (!(station->songFd = fopen(station->songName, "r"))){
		free(station->songName);
		free(station);
		terminate(failedOpenSong);
	}
	station->alive = 1;
	station->next = NULL;
	numOfStation++;
}
/* Play Music thread */
void *playStation(void * stationNum){
	char buf[BUFFER_SIZE];
	stationInfo *temp = stationList;
	int checkForStation = *(int *)stationNum;

	while(temp->stationNum != checkForStation)
		temp = temp->next;

	while(temp->alive){
		if(feof(temp->songFd))
			rewind(temp->songFd);
		fread(buf, 1, BUFFER_SIZE, temp->songFd);
		if (sendto(temp->socket, buf, BUFFER_SIZE, 0, (struct sockaddr *)&(temp->mcastIp), sizeof(temp->mcastIp)) < 0)
			terminate(failedSendTo);
		usleep(STREAM_RATE);
	}
	fclose(temp->songFd);
	pthread_exit((void *)temp->alive);
}
/* handle TCP Socket */
void createTcpSocket(uint16_t tcpPort){
	int reuse = 1;
	struct sockaddr_in serverAddress;

	if((welcomeSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		terminate(failedSocket);

	/* initialize address */
	/*---- Configure settings of the server address struct ----
	 * Setting zeros in serverAddr */
	bzero((char *) &serverAddress, sizeof(serverAddress));
	/* Address family = Internet */
	serverAddress.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddress.sin_port = htons(tcpPort);
	/* Set IP address */
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	if(setsockopt(welcomeSocket, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &reuse ,sizeof(reuse)) < 0)
		terminate(failedSetSockOpt);
	if (bind(welcomeSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		terminate(failedBind);
}
/* handle TCP Connections */
void handleTcpConnections(uint16_t tcpPort){
	fd_set fdSet;
	tcpConnectionInfo *temp, *endOfClientList;;
	struct sockaddr_in tempClientAddr;
	int inFd;
	socklen_t len;
	char *userComm;

	createTcpSocket(tcpPort);
	if (listen(welcomeSocket, 1) < 0)
		terminate(failedListen);

	printf("Welcome to the Radio Server!\n"
			"===============================\n"
			"Please type q to quit or p for current information\n");
	while(stationList->alive){
		/* initialize set FD */
		FD_ZERO(&fdSet);
		FD_SET(welcomeSocket, &fdSet);
		FD_SET(fileno(stdin), &fdSet);

		if((inFd = select(welcomeSocket+1, &fdSet, NULL, NULL, NULL)) < 0)
			terminate(failedSelect);
		/* input from user */
		else if (FD_ISSET(fileno(stdin), &fdSet)){
			userComm = readUserInput(stdin);
			if(strlen(userComm) > 1)
				userComm[0] = 1;
			switch(userComm[0]){
				case ('Q'):
				case ('q'):
					free(userComm);
					terminate(quit);
					break;
				case ('P'):
				case ('p'):
					free(userComm);
					printStats();
					break;
				default:
					free(userComm);
					printf("Invalid Command!\n");
					break;
			}
			printf("\nPlease type q to quit or p for current information\n");
		}
		/* input from socket */
		else if(FD_ISSET(welcomeSocket, &fdSet)){
			if (numOfClients > MAX_CLIENTS){
				printf("Sorry, client limit exceeded\n");
				break;
			}
			if(!(temp = (tcpConnectionInfo *)malloc(sizeof(tcpConnectionInfo))))
				terminate(failedMallocClient);
			len = sizeof(tempClientAddr);
			if((temp->socket = accept(welcomeSocket, (struct sockaddr *)&(temp->clientAddr),&len)) < 0){
				free(temp);
				terminate(failedAccept);
			}
			temp->alive = 1;
			temp->next = NULL;
			temp->state = waitForHello;
			if(!clientList){
				clientList = temp;
				endOfClientList = clientList;
			}
			else{
				endOfClientList->next = temp;
				endOfClientList = endOfClientList->next;
			}
			numOfClients++;
			pthread_create(&(temp->client), NULL, &handleClient,(void *)temp);
			pthread_detach(temp->client);
		}
	}
}
/* client handle thread */
void *handleClient(void *client){
	tcpConnectionInfo *radioClient = (tcpConnectionInfo *)client;
	fd_set fdSet;
	struct timeval timeout;
	int inSelect;

	while(radioClient->alive){
		FD_ZERO(&fdSet);
		FD_SET(radioClient->socket, &fdSet);
		switch (radioClient->state){
			case waitForHello:
				timeout.tv_sec = 0;
				timeout.tv_usec = HELLO_MSG_TO;
				break;
			case upSong:
				timeout.tv_sec = UPLOAD_TO;
				timeout.tv_usec = 0;
				break;
			default:
				timeout.tv_sec = EST_TO;
				timeout.tv_usec = 0;
				break;
		}


		if((inSelect = select(radioClient->socket + 1, &fdSet, NULL, NULL, &timeout)) < 0)
			terminate(failedSelect);

		/* Timeout Happened */
		if (!inSelect){
			sendInvMsg(radioClient,timeoutOccred);
			deleteClient(radioClient, timeoutOccred);
		}

		else if(FD_ISSET(radioClient->socket, &fdSet)){
			/* Uploading song for client */
			if(radioClient->state == upSong){
				downloadSong(radioClient);
				if(radioClient->state == uploaded){
					uploading = 0;
					sendNewStationMsg();
					radioClient->state = established;
				}
			}
			else
				handleMsg(radioClient);
		}
	}
	free(radioClient);
	numOfClients--;
	pthread_exit(NULL);
}
/* handle massage from clients */
void handleMsg(tcpConnectionInfo *client){
	char buf[BUFFER_SIZE]={0};

	if(recv(client->socket, buf, BUFFER_SIZE, 0) <= 0){
		deleteClient(client, communicationErr);
		return;
	}

	switch(buf[0]){
		case hello:{
			struct hello helloMsg;
			if (client->state > waitForHello){
				sendInvMsg(client, dupHello);
				deleteClient(client, dupHello);
				break;
			}
			helloMsg.commandType = buf[0];
			helloMsg.reserved = ntohs(*(uint16_t*)&buf[1]);
			if(helloMsg.commandType || helloMsg.reserved){
				sendInvMsg(client, helloErr);
				deleteClient(client, helloErr);
				break;
			}
			sendWelcomeMsg(client);
			client->state = established;
			break;
		}
		case askSong:{
			struct askSong songReq;
			if (client->state != established){
				sendInvMsg(client, invalid);
				deleteClient(client, invalid);
			}
			songReq.commandType = buf[0];
			songReq.stationNumber = ntohs(*(uint16_t *)&buf[1]);
			sendSongName(client, songReq.stationNumber);
			break;
		}
		case uploadSong:{
			struct upSong upSong;
			if (client->state != established){
				sendInvMsg(client, invalid);
				deleteClient(client, invalid);
			}
			upSong.commandType = buf[0];
			upSong.songSize = ntohl(*(uint32_t *)&buf[1]);
			upSong.songNameSize = buf[5];
			upSong.songName = (char *)malloc(sizeof(char)*upSong.songNameSize);
			strcpy(upSong.songName, &buf[6]);
			sendPermitMsg(client, checkPermit(upSong),upSong);
			break;
		}
		default:
			sendInvMsg(client, invalid);
			deleteClient(client, invalid);
			break;
	}
}
/* prints current server stats */
void printStats(){
	stationInfo *stationTemp = stationList;
	tcpConnectionInfo *clientTemp = clientList;
	int i = 1;

	/* prints stations */
	printf("Currently running stations are:\n");
	do{
		printf("Station %d: %s\n%s\n",stationTemp->stationNum,inet_ntoa(stationTemp->mcastIp.sin_addr),stationTemp->songName);
		stationTemp = stationTemp->next;
	}while(stationTemp);

	/* prints Clients */
	if(!numOfClients)
		printf("There are no clients connected at the moment\n");
	else{
		printf("Here is a list of our clients:\n");
		do{
			printf("Client %d with an IP address: %s\n",i,inet_ntoa(clientTemp->clientAddr.sin_addr));
			clientTemp = clientTemp->next;
			i++;
		}while(clientTemp);
	}
}
/* deletes client */
void deleteClient(tcpConnectionInfo *client, int error){
	tcpConnectionInfo *temp;
	pthread_t tempT;

	switch(error){
		case communicationErr:
			printf("Client suddenly disconnected, Client deleted\n");
			break;
		case invalid:
			printf("Invalid massage received\n");
			break;
		case timeoutOccred:
			printf("Timeout occurred!, Client deleted\n");
			break;
		case dupHello:
			printf("Duplicated hello massage received!, Client deleted\n");
			break;
		case helloErr:
			printf("Expected hello massage, Client deleted\n");
			break;
		case stationErr:
			printf("Wrong station number requested, Client deleted\n");
			break;
		default:
			break;
	}
	if (clientList == client)
		clientList = client->next;

	else{
		do{
			if(temp == client){
				temp = client->next;
				break;
			}
			temp = temp->next;
		}while(temp);
	}
	client->alive = 0;
	tempT = client->client;
	close(client->socket);
	pthread_cancel(tempT);
}
/* sends to client invalid msg */
void sendInvMsg(tcpConnectionInfo *client, int error){
	char *buf, *repStr;
	int size;
	switch(error){
		case helloErr:
			repStr = "Didn't receive hello massage\n";
			break;
		case dupHello:
			repStr = "Received more then 1 hello massage\n";
			break;
		case invalid:
			repStr = "Unexpected massage received\n";
			break;
		case sizeErr:
			repStr = "Message received with wrong size\n";
			break;
		case stationErr:
			repStr = "Requested station doesn't exist\n";
			break;
		case uploatTo:
			repStr = "Timeout occurred while uploading song\n";
			break;
		case timeoutOccred:
			repStr = "Timeout occurred!\n";
			break;
		default:
			repStr = "Unknown error occurred\n";
	}
	size = 2 + strlen(repStr);
	buf = (char *)malloc(sizeof(char)*size +1);
	buf[0] = invalidCommand;
	buf[1] = strlen(repStr);
	strcpy(&buf[2], repStr);
	if (send(client->socket, &buf, size, 0) < 0){
		free(buf);
		deleteClient(client, communicationErr);
	}
	free(buf);
}
/* sends welcome message to client */
void sendWelcomeMsg(tcpConnectionInfo *client){
	char buf[9] = {0};

	buf[0] = welcome;
	*(uint16_t *)&buf[1] = htons(numOfStation);
	*(uint32_t *)&buf[3] = htonl(stationList->mcastIp.sin_addr.s_addr);
	*(uint16_t *)&buf[7] = stationList->mcastIp.sin_port;

	if(send(client->socket, buf, 9, 0) < 0)
		deleteClient(client, communicationErr);
}
/* sends requested song name */
void sendSongName(tcpConnectionInfo *client, uint16_t stationNum){
	char *buf, *onlySongName;
	int size;
	stationInfo *temp = stationList;

	/* invalid station number */
	if(stationNum > numOfStation){
		sendInvMsg(client, stationErr);
		deleteClient(client, stationErr);
	}
	/* Find station from station list */
	do{
		if(temp->stationNum == stationNum)
			break;
		temp = temp->next;
	}while(temp);

	onlySongName = getSongName(temp->songName);
	size = 2 + strlen(onlySongName);
	buf = (char *)malloc(sizeof(char)*size+1);
	buf[0] = announce;
	buf[1] = strlen(onlySongName);
	strcpy(&buf[2], onlySongName);

	if(send(client->socket, buf, size, 0) < 0){
		free(buf);
		deleteClient(client, communicationErr);
	}
	free(buf);
}
/* cuts directory from song name */
char *getSongName(char *fullSongPath){
	int size = strlen(fullSongPath);
	for(;size >=0; size--)
		if(fullSongPath[size] == '/')
			return &fullSongPath[size+1];
	return fullSongPath;
}
/* returns if permit to upload song */
int checkPermit(struct upSong upSong){
	stationInfo *tempStation = stationList;

	/* check no one else is uploading */
	if (uploading){
		printf("Another client currently uploading\n");
		return 0;
	}

	/* check correct file size */
	if ((upSong.songSize < 2000) || (upSong.songSize > 10*MEGA)){
		printf("File size is incorrect\n");
		return 0;
	}

	/* check no duplicated song name */
	while(tempStation){
		if(!strcmp(upSong.songName, tempStation->songName) || fopen(upSong.songName,"r")){
			printf("Song with such name already exists\n");
			return 0;
		}
		tempStation = tempStation->next;
	}
	return 1;
}
/* sends the permit result */
void sendPermitMsg(tcpConnectionInfo *client, int permit, struct upSong upSong1){
	char buf[2];

	buf[0] = permitSong;
	buf[1] = permit;
	if (send(client->socket, &buf, 2, 0) < 0)
		deleteClient(client, communicationErr);

	if(permit){
		uploading = permit;
		client->state = upSong;
		newSongName = upSong1.songName;
		newSongSize = upSong1.songSize;
		printf("Starting to download a new song.\n");
	}
}
/* download song from client */
void downloadSong(tcpConnectionInfo *client){
	char buf[BUFFER_SIZE] = {0};
	int bytesRec;
	FILE *newSongFd = fopen(newSongName, "a");

	if((bytesRec = recv(client->socket, buf, BUFFER_SIZE, 0)) < 0){
		fclose(newSongFd);
		deleteClient(client, communicationErr);
	}
	fwrite(buf, sizeof(char), bytesRec, newSongFd);
	/* check if downloaded all file */
	if(ftell(newSongFd) == newSongSize)
		client->state = uploaded;
	fclose(newSongFd);
}
/* sends new station massage */
void sendNewStationMsg(){
	stationInfo *newRadioStation, *tempStation = stationList;
	tcpConnectionInfo *tempClient = clientList;
	char buf[3]={0};
	char *mcastIP = inet_ntoa(stationList->mcastIp.sin_addr);

	if(!(newRadioStation = (stationInfo *) malloc(sizeof(stationInfo)))){
		free(newSongName);
		terminate(failedToMallocStation);
	}
	addStation(newRadioStation, newSongName, mcastIP, ntohs(stationList->mcastIp.sin_port));
	free(newSongName);
	while(tempStation){
		if(!tempStation->next){
			tempStation->next = newRadioStation;
			break;
		}
		tempStation = tempStation->next;
	}
	pthread_create(&(newRadioStation->station), NULL, &playStation, (void *)&newRadioStation->stationNum);

	buf[0] = newStation;
	*(uint16_t *)&buf[1] = htons(newRadioStation->stationNum);

	do{
		if(send(tempClient->socket, buf, 3, 0) < 0)
			deleteClient(tempClient, communicationErr);
		tempClient = tempClient->next;
	}while(tempClient);
	printf("New station added to the radio!\n");
}
/* terminate functio */
void terminate(int error){
	char *repStr;
	switch (error){
		case failedToMallocStation:
			repStr = "Failed to allocate memory for station\n";
			break;
		case failedToMallocStr:
			repStr = "Failed to allocate memory for string\n";
			break;
		case failedSocket:
			repStr = "Failed in socket function\n";
			break;
		case failedBind:
			repStr = "Failed in bind function\n";
			break;
		case failedSetSockOpt:
			repStr = "Failed in setsockopt function\n";
			break;
		case failedOpenSong:
			repStr = "Failed in fopen function\n";
			break;
		case failedSendTo:
			repStr = "Failed in sendto function\n";
			break;
		case failedListen:
			repStr = "Failed in listen function\n";
			break;
		case failedSelect:
			repStr = "Failed in select function\n";
			break;
		case quit:
			repStr = "We are terminating everything. Good Day :)\n";
			break;
		case failedMallocClient:
			repStr = "Failed to allocate memory for client\n";
			break;
		case failedAccept:
			repStr = "Failed in accept function\n";
			break;
		}
	freeAllClients();
	freeAllStations();
	if(welcomeSocket)
		close(welcomeSocket);
	errorFunc(repStr);
}
/* frees all stations */
void freeAllStations(){
	stationInfo *temp;
	while(stationList){
		temp = stationList->next;
		stationList->alive = 0;
		pthread_join(stationList->station,NULL);
		close(stationList->socket);
		free(stationList->songName);
		free(stationList);
		stationList = temp;
	}
}
/* frees all clients */
void freeAllClients(){
	while(clientList){
		deleteClient(clientList, quit);
	}
}
/* reads input from user */
char *readUserInput(FILE *fp) {
//The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0, size=USER_INPUT_BUF;
    str = (char *)malloc(sizeof(char)*size);
    if(!str) terminate(failedToMallocStr);
    while(EOF!=(ch=fgetc(fp)) && ch != '\n'){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(char)*(size+=16));
            if(!str)return str;
        }
    }
    str[len++]='\0';
    return realloc(str, sizeof(char)*len);
}
