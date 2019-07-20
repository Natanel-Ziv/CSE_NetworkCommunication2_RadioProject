/*
 ============================================================================
 Name        : serverFunc.h
 Author      : GodisC00L
 Description : header for server side in internet radio project
 ============================================================================
 */

#ifndef SERVERFUNC_H_
#define SERVERFUNC_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "serverCommands.h"


#define TTL	10
#define BUFFER_SIZE 1024
#define STREAM_RATE 62500
#define MAX_CLIENTS	100
#define TRUE	1
#define HELLO_MSG_TO	300000
#define EST_TO	3600
#define MEGA	1000000
#define UPLOAD_TO	3
#define	USER_INPUT_BUF	10

enum states {off, waitForHello, established, upSong, uploaded};
enum clientMsg {hello, askSong,uploadSong};
enum invMsgErr{helloErr, dupHello, invalid, sizeErr, stationErr, uploatTo, timeoutOccred, communicationErr};
enum serverMsgType{welcome, announce, permitSong, invalidCommand, newStation};
enum serverErrors{failedToMallocStation, failedToMallocStr, failedSocket, failedBind, failedSetSockOpt,
	failedOpenSong, failedSendTo, failedListen, failedSelect, quit, failedMallocClient, failedAccept};

/* Function Declarations */
void errorFunc(char *msg);
void addStation(stationInfo *station, const char* songName, const char* mcastIP, uint16_t udpPort);
void *playStation(void * stationNum);
void createTcpSocket(uint16_t tcpPort);
void handleTcpConnections(uint16_t tcpPort);
void printStats();
void *handleClient(void *client);
void handleMsg(tcpConnectionInfo *client);
void deleteClient(tcpConnectionInfo *client, int error);
void sendInvMsg(tcpConnectionInfo *client, int error);
void sendWelcomeMsg(tcpConnectionInfo *client);
void sendSongName(tcpConnectionInfo *client, uint16_t stationNum);
char *getSongName(char *fullSongPath);
int checkPermit(struct upSong upSong);
void sendPermitMsg(tcpConnectionInfo *client, int permit, struct upSong upSong);
void downloadSong(tcpConnectionInfo *client);
void sendNewStationMsg();
void terminate(int error);
void freeAllStations();
void freeAllClients();
char *readUserInput(FILE *fp);


/* Variable Declaration */
static int uploading = 0, welcomeSocket = 0, numOfClients = 0;
static uint16_t numOfStation = 0;
static uint32_t newSongSize;
extern stationInfo *stationList;
extern tcpConnectionInfo *clientList;
static char* newSongName;

#endif /* SERVERFUNC_H_ */
