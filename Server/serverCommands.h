/*
 ============================================================================
 Name        : serverCommands.h
 Author      : GodisC00L
 Description : header for server side commands in internet radio project
 ============================================================================
 */
#ifndef SERVERCOMMANDS_H_
#define SERVERCOMMANDS_H_

/* Client structs */
struct hello{
    uint8_t commandType;
    uint16_t reserved;
};

struct askSong{
    uint8_t commandType;
    uint16_t stationNumber;
};

struct upSong{
    uint8_t commandType;
    uint32_t songSize; //in bytes
    uint8_t songNameSize;
    char *songName;
};

/* Server Structs */
struct welcome{
    uint8_t replyType;
    uint16_t numStations;
    uint32_t multicastGroup;
    uint16_t portNumber;
};

struct announce{
    uint8_t replyType;
    uint8_t songNameSize;
    char *songName;
};

struct permitSong{
    uint8_t replyType;
    uint8_t permit;
};

struct invalidCommand{
    uint8_t replyType;
    uint8_t replyStringSize;
    char *replyString;
};

struct newStations{
    uint8_t replyType;
    uint16_t newStationNumber;
};

typedef struct tcpConnectionInfo{
	int socket;
	int state;
	struct sockaddr_in clientAddr;
	pthread_t client;
	int alive;
	struct tcpConnectionInfo *next;
}tcpConnectionInfo;

typedef struct stationInfo{
	char *songName;
	FILE *songFd;
	uint16_t stationNum;
	struct sockaddr_in mcastIp;
	pthread_t station;
	int socket;
	struct stationInfo *next;
	int alive;
} stationInfo;
#endif /* SERVERCOMMANDS_H_ */
