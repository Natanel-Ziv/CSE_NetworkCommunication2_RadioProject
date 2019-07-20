/*
 ============================================================================
 Name        : clientFunc.c
 Author      : GodisC00L
 Description : functions for client side in internet radio project
 ============================================================================
 */
#include "clientFunc.h"

/* prints errors */
void error(char *msg) {
    perror(msg);
    exit(errno);
}

/* Opens a TCP connection */ 
void openTcpSock(char* hostname, uint16_t portn){
    struct sockaddr_in serverAddr;
    /* Open TCP socket */
    if( (clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        free(hostname);
        pthread_exit((void *)failedSocket);
    }

    /*---- Configure settings of the server address struct ----
     * Setting zeros in serverAddr */
    bzero((char *) &serverAddr, sizeof(serverAddr));
    /* Address family = Internet */
    serverAddr.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    serverAddr.sin_port = htons(portn);
    /* Set IP address */
    serverAddr.sin_addr.s_addr = inet_addr(hostname);

    /*---- Connect the socket to the server using the address struct ----*/
    if (connect(clientSocket, (struct sockaddr *) &serverAddr,
                sizeof(serverAddr)) < 0) {
        free(hostname);
        pthread_exit((void *)failedBind);
    }
    free(hostname);
}

/* Sends hello message */
void helloMsg(){
    char data[3] = {0};
    if(send(clientSocket,data,3,0) < 0)
        pthread_exit((void *)failedSendTCP);
    state = waitForWelcome;
}
/* Prints menu */
void menu() {
    printf("\nMenu:\n"
           "======\n"
           "Press a number to switch to the desired station.\n"
           "s - upload a song.\n"
           "q - quit.\n");
}
/* Control radio I/O */
void *radioControl(void *clientControl) {
    struct timeval timeout;
    fd_set fdSet;
    int selFd;
    char *user;
    uint16_t stationNum;

    while(TRUE){
        /* Set timeout for each case */
        switch (state){
            case upload:
                timeout.tv_sec = THREE_SEC_TO;
                timeout.tv_usec = 0;
                break;
            case established:
            	menu();
                timeout.tv_sec = INF_TO;
                timeout.tv_usec = 0;
                break;
            default:
                timeout.tv_sec = DEFAULT_TO;
                timeout.tv_usec = 0;
                break;
        }

        /* Initialize fd set to our optional inputs */
        FD_ZERO(&fdSet);
        FD_SET(fileno(stdin), &fdSet);
        FD_SET(clientSocket, &fdSet);

        /* run select() if returned value less then 0 then ERROR */
        if ((selFd = select(clientSocket+1,&fdSet, NULL, NULL, &timeout)) < 0){
            close(clientSocket);
            perror("Error in select");
        }

        /* if returned value = 0 - timeout occurred */
        if(!selFd){
            close(clientSocket);
            perror("Timeout occurred");
        }

            /* if returned value is positive handle fd */
        else{
            /* Handle keyboard input */
            if (FD_ISSET(fileno(stdin), &fdSet)){
                fflush(stdin);
                user = readUserInput(stdin);
				/* Check input correctness */
                if(strlen(user) > 1)
                	if(!isDigit(user))
                		user[0] = '!';
                switch (user[0]){
                    case ('S'):
                    case('s'):
                        free(user);
                        askUploadSong();
                        break;
                    case ('Q'):
                    case ('q'):
                        free(user);
                        pthread_exit((void *)quit);
                        break;
                    default: {
						/* if input is number then change station */
                        struct askSong songReq;
                        if(isDigit(user)){
							stationNum = (uint16_t) atoi(user);
							free(user);
							if (stationNum >= 0 && stationNum < radioInfo.numStations) {
								if (stationNum == currentStation){
									printf("You are already listening to the station\n");
									break;
								}
								songReq.commandType = 1;
								songReq.stationNumber = stationNum;
								switchStation(stationNum);
								reqSong(songReq);
							}
							else
								printf("Invalid station number!\nMax station number is %d\n", radioInfo.numStations);
                        }
                        else{
                        	free(user);
                        	printf("Invalid command!\n");
                        }
                        break;
                    }
                }
            }

                /* Handle socket input */
            else if (FD_ISSET(clientSocket, &fdSet)){
                handleMsg();
            }
        }
    }
}
/* sends upload song request */
void askUploadSong() {
    FILE *songFd;
    char data[BUF_SIZE];

    printf("Please enter song name\n");
    song.commandType = 2;
    song.songName = readUserInput(stdin);
    song.songNameSize = (uint8_t) strlen(song.songName);

	/* case cant open file */
    if (!(songFd = fopen(song.songName,"r"))){
    	free(song.songName);
        printf("Error Opening Song\n");
        return;
    }

	/* check that file size is legal */
    fseek(songFd, 0L, SEEK_END);
    song.songSize = (uint32_t) ftell(songFd);
    rewind(songFd);
    fclose(songFd);
    if (song.songSize < 2000 || song.songSize > 10*MEGA){
        printf("Invalid song size\n");
        return;
    }

    data[0] = song.commandType;
    *(uint32_t*)&data[1] = htonl(song.songSize);
    data[5] = song.songNameSize;
    strncpy(data+6,song.songName,song.songNameSize);

    if(send(clientSocket, &data, 6+song.songNameSize, 0) < 0){
        printf("Failed to send upload song request\n");
        free(song.songName);
        pthread_exit((void *)failedSendTCP);
    }
    state = waitForPermition;
}
/* reads input from terminal (stdin) */
char *readUserInput(FILE *fp) {
//The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0, size=USER_INPUT_BUF;
    str = (char *)malloc(sizeof(char)*size);
    if(!str) pthread_exit((void *)failedMalloc);
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
/* handle messages received from server */
void handleMsg() {
    char buf[BUF_SIZE];

    if(recv(clientSocket, buf, BUF_SIZE, 0) <= 0)
    	pthread_exit((void *)unexpectedTermination);

    switch (buf[0]){
        case welcome:
            if (state != waitForWelcome)
                pthread_exit((void *)unexpectedMsgWelcome);

            radioInfo.replyType = (uint8_t) buf[0];
            radioInfo.numStations = ntohs(*(uint16_t*)&buf[1]);
            radioInfo.multicastGroup = ntohl(*(uint32_t *)&buf[3]);
            radioInfo.portNumber = ntohs(*(uint16_t *)&buf[7]);
            connectToRadio();
            break;
        case announce: {
            struct announce announceMsg;
            if (state != waitForAnounce)
                pthread_exit((void *)waitForAnounce);
            announceMsg.replyType = (uint8_t) buf[0];
            announceMsg.songNameSize = (uint8_t) buf[1];
            announceMsg.songName = (char *) calloc(announceMsg.songNameSize + 1,sizeof(char));
            strncpy(announceMsg.songName, &buf[2], announceMsg.songNameSize);
            printf("Current song is: %s\n", announceMsg.songName);
            free(announceMsg.songName);
            state = established;
            break;
        }
        case permitSong:{
            struct permitSong permitMsg;
            if(state != waitForPermition)
                pthread_exit((void *)unexpectedMsgPermit);
            permitMsg.replyType = (uint8_t) buf[0];
            permitMsg.permit = (uint8_t) buf[1];
            if(permitMsg.permit){
                uploadSong();
            }
            else {
            	free(song.songName);
                printf("No permission to upload song at the moment\n");
                state = established;
            }
            break;
        }
        case invalidCommand:{
            struct invalidCommand invComMsg;
            invComMsg.replyType = (uint8_t) buf[0];
            invComMsg.replyStringSize = (uint8_t) buf[1];
            invComMsg.replyString = (char *)malloc((sizeof(char)*invComMsg.replyStringSize)+1);
            strncpy(invComMsg.replyString, &buf[2], invComMsg.replyStringSize);
            printf("Invalid Command!\n"
                   "Error is: %s\n",invComMsg.replyString);
            free(invComMsg.replyString);
            pthread_exit((void *)invalidComm);
            break;
        }
        case newStation:{
            struct newStations newStationMsg;
            if (state != waitForNewStation && state != established)
                pthread_exit((void *)unexpectedMsgNewStation);
            newStationMsg.replyType = (uint8_t) buf[0];
            newStationMsg.newStationNumber = ntohs(*(uint16_t*)&buf[1]);
            printf("New Station has been added!\n"
                   "Station number is: %d\n",newStationMsg.newStationNumber);
            radioInfo.numStations += 1;
            state = established;
            break;
        }
        default:
            pthread_exit((void *)unexpectedMsgUnknown);
            break;
    }
}
/* connects to the mulsicast */
void connectToRadio() {
    struct sockaddr_in radioAddr;
    struct in_addr mcastIP;
    struct askSong requestSong;
    mcastIP.s_addr = radioInfo.multicastGroup;
    printf("Welcome!\n"
           "========\n"
           "Establishing connection to:\n"
           "Multicast IP: %s\n"
           "Port: %d\n"
           "Number of stations is: %d\n",inet_ntoa(mcastIP),radioInfo.portNumber, radioInfo.numStations);
    /* Create UDP socket */
    if ((radioSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        pthread_exit((void *)failedSocket);

    /*---- Configure settings of the server address struct ----
     * Setting zeros in serverAddr */
    bzero((char *) &radioAddr, sizeof(radioAddr));
    /* Address family = Internet */
    radioAddr.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    radioAddr.sin_port = htons(radioInfo.portNumber);
    /* Set IP address */
    radioAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(radioSocket, (struct sockaddr *)&radioAddr, sizeof(radioAddr)) < 0)
        pthread_exit((void *)failedBind);

    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = mcastIP.s_addr;

    if(setsockopt(radioSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        pthread_exit((void *)failedSetSockOpt);

    state = established;
    requestSong.commandType = 1;
    requestSong.stationNumber = 0;
    currentStation = requestSong.stationNumber;
    reqSong(requestSong);
    pthread_create(&radio, NULL, &radioThread, NULL);
}
/* thread function that plays the music */
void *radioThread(void *radio) {
    char songBuf[BUF_SIZE];
    FILE *songToPlay;
    socklen_t length;
    int bytesRec;
    struct sockaddr_in addr;

    close_radio = 0;
    pthread_detach(pthread_self());
    songToPlay = popen("play -t mp3 -> /dev/null 2>&1", "w");
    length = sizeof(addr);
    while(!close_radio){
        if ((bytesRec = recvfrom(radioSocket, songBuf, BUF_SIZE, 0, (struct sockadddr *) &addr, &length)) < 0)
            terminate(failedReciveMusic);
        if (bytesRec == BUF_SIZE)
            fwrite(songBuf, sizeof(char), bytesRec, songToPlay);
        if (close_radio)
        	pthread_exit(NULL);
    }
    pthread_exit(NULL);
}
/* asks server for the song name */
void reqSong(struct askSong songReq) {
    uint8_t buf[3];
    buf[0] = songReq.commandType;
    *(uint16_t*)&buf[1] = htons(songReq.stationNumber);

    printf("Asking for station %d\n",songReq.stationNumber);
    if (send(clientSocket, buf, sizeof(buf), 0) < 0){
        close(clientSocket);
        close(radioSocket);
        perror("Failed to ask station");
    }
    state = waitForAnounce;
}
/* uploads the song to the server */
void uploadSong() {
    FILE *songFd;
    char buf[BUF_SIZE +1];
    int bytesSent = 0, curSend, numOfBytes = song.songSize;

    if((songFd = fopen(song.songName, "r")) == NULL){
    	free(song.songName);
    	pthread_exit((void *)failedOpenFile);
    }
    state = upload;
    printf("Uploading song!\n");

    while(numOfBytes > 0){
    	memset(buf, 0, BUF_SIZE+1);
    	fread(buf, 1, BUF_SIZE, songFd);
		if (numOfBytes < BUF_SIZE){
			if ((curSend = send(clientSocket, buf, numOfBytes, 0)) < 0)
				pthread_exit((void *)failedSendTCP);
		}
		else {
			if ((curSend = send(clientSocket, buf, BUF_SIZE, 0)) < 0)
				pthread_exit((void *)failedSendTCP);
		}

		bytesSent += curSend;
		numOfBytes -= curSend;
		progressBar("Uploading ", song.songSize, bytesSent);
		usleep(RATE_TO);
    }
    state = waitForNewStation;
    fclose(songFd);
}
/* progress bar for the upload */
void progressBar(char label[], int totalSize, int byteSent) {
	//progress width
	const int pwidth = 72;

	//minus label len
	int i;
	int width = pwidth - strlen(label);
	int pos = ( byteSent * width ) / totalSize ;

	int percent = ( byteSent * 100 ) / totalSize;
	printf( "%s[", label );

	//fill progress bar with =
	for ( i = 0; i < pos; i++ )
		printf( "%c", '=' );

	//fill progress bar with spaces
	printf( "%*c", width - pos + 1, ']' );
	printf( " %3d%% \r", percent );
	fflush(stdout);
}
/* terminate function */
void terminate(int error) {
	if (!close_radio){
		close_radio = 1;
		pthread_cancel(radio);
	}

	switch (error){
    	case quit:
    		closeSockets();
    		printf("Everything was terminated\n");
    		break;
    	case failedMalloc:
    		closeSockets();
    		perror("Failed to use malloc");
    		break;
        case unexpectedMsgWelcome:
            closeSockets();
            perror("Unexpected message! Message received is welcome");
            break;
        case unexpectedMsgAnnounce:
            closeSockets();
            perror("Unexpected message! Message received is announce");
            break;
        case unexpectedMsgPermit:
            closeSockets();
            perror("Unexpected message! Message received is permit");
            break;
        case invalidComm:
            closeSockets();
            perror("");
            break;
        case unexpectedMsgNewStation:
            closeSockets();
            perror("Unexpected message! Message received is new station");
            break;
        case unexpectedMsgUnknown:
            closeSockets();
            perror("Unexpected message! Message received is unknown");
            break;
        case failedSocket:
            closeSockets();
            perror("Failed to create socket");
            break;
        case failedBind:
            closeSockets();
            perror("Failed to bind socket");
            break;
        case failedSetSockOpt:
            closeSockets();
            perror("Failed to set UDP socket opetions");
            break;
        case failedReciveMusic:
            closeSockets();
            perror("Failed to receive music from UDP socket");
            break;
        case failedSendTCP:
            closeSockets();
            perror("Failed to send data on TCP socket");
            break;
        case failedOpenFile:
            closeSockets();
            perror("Failed open file");
            break;
        case failedToReadFile:
            closeSockets();
            perror("Failed to read file");
            break;
        case unexpectedTermination:
			closeSockets();
			perror("Server disconnected");
			break;
        default:
            break;
    }
}
/* closes open sockets */
void closeSockets() {
    if (clientSocket)
        close(clientSocket);
    if(radioSocket)
        close(radioSocket);
}
/* switches the station (change mcastIP) */
void switchStation(int stationNum){
	int offsetForStation = stationNum - currentStation;

	/* Drop previous station */
	if (setsockopt(radioSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		pthread_exit((void *)failedSetSockOpt);

	mreq.imr_multiaddr.s_addr = htonl(ntohl(mreq.imr_multiaddr.s_addr) + offsetForStation);
	if (setsockopt(radioSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		pthread_exit((void *)failedSetSockOpt);

	currentStation = stationNum;
}
/* checks if string is digits */
int isDigit(char *str){
	int i;
	for(i = 0; i < strlen(str); i++)
		if(str[i] > '9' || str[i] < '0')
			return 0;
	return 1;
}
