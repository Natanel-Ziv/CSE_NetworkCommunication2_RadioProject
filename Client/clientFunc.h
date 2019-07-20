/*
 ============================================================================
 Name        : clientFunc.c
 Author      : GodisC00L
 Description : header for client side functions in internet radio project
 ============================================================================
 */

#ifndef CLIENTFUNC_H_
#define CLIENTFUNC_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "clientCommands.h"

#define TWO_SEC_TO  2
#define THREE_SEC_TO    3
#define DEFAULT_TO  300000
#define INF_TO  3600 //Hour
#define TRUE 1
#define USER_INPUT_BUF  10
#define MEGA    1000000
#define BUF_SIZE    1024
#define RATE_TO 8000


enum states {off, waitForWelcome, established, waitForAnounce, waitForPermition, upload, waitForNewStation};
enum serverMsgs{welcome, announce, permitSong, invalidCommand, newStation};
enum errors{unexpectedMsgWelcome, unexpectedMsgAnnounce, unexpectedMsgPermit, invalidComm, unexpectedMsgNewStation, unexpectedMsgUnknown, failedSocket,
        failedBind, failedSetSockOpt, failedReciveMusic, failedSendTCP, failedOpenFile, failedToReadFile, failedMalloc, quit, unexpectedTermination};


/* Function Declarations */
void error(char *msg);
void openTcpSock(char* hostname, uint16_t portn);
void helloMsg();
void menu();
void *radioControl(void *clientControl);
char *readUserInput(FILE *fp);
void askUploadSong();
void handleMsg();
void connectToRadio();
void *radioThread(void *radio);
void reqSong(struct askSong songReq);
void uploadSong();
void progressBar(char label[], int totalSize, int byteSent);
void terminate(int error);
void closeSockets();
void switchStation(int stationNum);
int isDigit(char *str);


/* Variable Declaration */
static int state, clientSocket = 0, radioSocket = 0, currentStation, close_radio = -1;
pthread_t clientControl, radio;
struct upSong song;
struct welcome radioInfo;
struct ip_mreq mreq;


#endif /* CLIENTFUNC_H_ */
