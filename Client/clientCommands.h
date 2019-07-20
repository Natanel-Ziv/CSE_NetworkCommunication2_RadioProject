/*
 ============================================================================
 Name        : clientFunc.c
 Author      : GodisC00L
 Description : header for client side commands in internet radio project
 ============================================================================
 */

#ifndef CLIENTCOMMANDS_H_
#define CLIENTCOMMANDS_H_

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

#endif /* CLIENTCOMMANDS_H_ */
