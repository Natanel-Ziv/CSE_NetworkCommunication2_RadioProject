/*
 ============================================================================
 Name        : main.c
 Author      : GodisC00L
 Description : main for client side in internet radio project
 ============================================================================
 */
#include "clientFunc.h"

int main(int argc, char *argv[]) {
    uint16_t portn;
    char* hostname;
    int size, exitStatus;

    /* Check if we have all necessary arguments are there*/
    if (argc < 3) {
        errno = EIO;
        error("Usage: hostname, port");
    }

    size = strlen(argv[1]);
    hostname = (char *)calloc(size+1, sizeof(char));
    memcpy(hostname, argv[1], size);
    portn = (uint16_t) atoi(argv[2]);

	state = off;
	/* Open a tcp connection with the server */
    openTcpSock(hostname, portn);
	/* Sends hello msg */
    helloMsg();	
	/* create thread for the TCP connection */
    pthread_create(&clientControl, NULL, &radioControl, NULL);
    pthread_join(clientControl, &exitStatus);
    terminate((int *)exitStatus);
    return 0;
}
