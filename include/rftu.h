#ifndef __RFTU_H__
#define __RFTU_H__

/*-------------------*/
/*-----LIBRARIES-----*/
/*-------------------*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

/*-------------------*/
/*------MACROS-------*/
/*-------------------*/
// RFTU Commands IDs
#define RFTU_CMD_NULL      0x00
#define RFTU_CMD_INIT      0x01
#define RFTU_CMD_DATA      0x02
#define RFTU_CMD_READY     0x03
#define RFTU_CMD_NOSPACE   0x04
#define RFTU_CMD_ACK 	   0x05
#define RFTU_CMD_ERROR 	   0x06
#define RFTU_CMD_COMPLETED 0x07

// RFTU Default Settings
#define RFTU_SEGMENT_SIZE   1024
#define RFTU_PORT 			8888
#define RFTU_TIMEOUT		3
#define RFTU_MAX_RETRY		10
#define RFTU_WINDOW_SIZE	8

// RFTU Control Flags
extern unsigned char flagServer;
extern unsigned char flagVerbose;
extern unsigned char flagFile;
extern unsigned char flagIP;

// RFTU Data Packet
struct rftuDataPkt {
	unsigned char 	cmd;						/* if cmd = DATA or INIT, size and data is available */
	unsigned char 	id;
	unsigned int  	seq;
	unsigned short 	size;
	unsigned char 	data[RFTU_SEGMENT_SIZE];
};

// RFTU Command Packet
struct rftuCmdPkt {
	unsigned char 	cmd;
	unsigned char 	id;
	unsigned int  	seq;
	unsigned short 	size;
};

// RFTU File Info
struct fileInfo {
	char 				fileName[256];
	unsigned long int  	fileSize;
};

// RFTU Sending Windows
struct windows {
	unsigned char sent;
	unsigned char ack;
	struct rftuDataPkt pkt;
};

// RFTU Global Variables
extern char 				rftuFilename[256];
extern unsigned long int  	rftuFilesize;
extern char 				rftuIP[20];
extern unsigned short 		rftuID;

/*-----------------------------*/
/*-----FUNCTION PROTOTYPES-----*/
/*-----------------------------*/
// Global functions
void 			dispHelp(void);
unsigned char 	checkIP(char *);
unsigned char 	checkFileExist(char *);
void 			socketError(int , char *);

// Sender functions (rftuSender.c)
unsigned char 		rftuSender(void);
char*				getFileName(char *);
unsigned long int  	getFileSize(char *);
void 				addPkt(struct windows *, unsigned char , int , unsigned int *);
void 				removePkt(struct windows *, unsigned char , unsigned int );
void 				sendPkt(struct windows *, unsigned char , int , struct sockaddr_in *, unsigned char );

// Receiver functions (rftuReceiver.c)
unsigned char 	rftuReceiver(void);

#endif
