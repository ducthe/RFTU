/*
*   Filename    : rftu.h
*   Author      : Issac, Kevin, Peter, Richard (OCTO Team)
*   Date        : 06-Sep-2016
*/

#ifndef __RFTU_H__
#define __RFTU_H__

/*-------------------*/
/*-----LIBRARIES-----*/
/*-------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <libgen.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>

// RFTU Commands IDs
#define RFTU_CMD_NULL      0x00
#define RFTU_CMD_INIT      0x01
#define RFTU_CMD_DATA      0x02
#define RFTU_CMD_READY     0x03
#define RFTU_CMD_NOSPACE   0x04
#define RFTU_CMD_ACK 	   0x05
#define RFTU_CMD_ERROR 	   0x06
#define RFTU_CMD_COMPLETED 0x07

// RFTU Returned Value
#define RFTU_RET_OK 		0
#define RFTU_RET_ERROR     -1

// RFTU Logics
#define YES 1
#define NO  0

// RFTU Default Settings
#define RFTU_FRAME_SIZE     15950
#define RFTU_TIMEOUT		3
#define RFTU_WELCOME_PORT   8888
#define RFTU_MAX_RETRY		10
#define RFTU_WINDOW_SIZE	8

#define THREAD_NUMBER       8

// RFTU Control Flags
extern unsigned char ucFlagServer;
extern unsigned char ucFlagVerbose;
extern unsigned char ucFlagFile;
extern unsigned char ucFlagIP;
extern unsigned char ucFlagACKDrop;
extern unsigned char ucFlagPacketDrop;

// RFTU Thread Numbers
extern unsigned int unThreadNumber;

// RFTU Data Package
struct g_stRFTUPacketData {
	unsigned char 	ucCmd;						/* if cmd = DATA or INIT, size and data is available */
	unsigned char 	ucID;
	unsigned int  	unSeq;
	unsigned short 	usSize;
	unsigned char 	ucData[RFTU_FRAME_SIZE];
};

// RFTU Command Package
struct g_stRFTUPacketCmd {
	unsigned char 	ucCmd;
	unsigned char 	ucID;
	unsigned int  	unSeq;
	unsigned short 	usSize;
};

// RFTU File Info
struct g_stFileInfo {
	char 				cFileName[256];
	unsigned long int  	ulFileSize;
};

// RFTU window
struct g_stWindows {
	unsigned char ucSent;
	unsigned char ucAck;
	struct g_stRFTUPacketData stRFTUPacketData;
};

struct g_stSenderParam {
    int nPortNumber;
    unsigned int unWindowSize;
    int nFilePointerStart;
    int nFileSize;
    char cThreadID;
};

struct g_stReceiverParam {
    int nPortNumber;
    int fd;
    int nFilePointerStart;
    int nFileSize;
    char cThreadID;
};

struct g_stPortInfo {
    unsigned char ucNumberOfPort;
    int nPortNumber[THREAD_NUMBER];
};

// RFTU Global Variables
extern char 				cRFTUFileName[256];
extern unsigned long int  	ulRFTUFileSize;
extern char 				cRFTUIP[20];
extern unsigned short 		usRFTUid;
extern unsigned char        ucAckLossRate;
extern unsigned char        ucPacketLossRate;


/*-----------------------------*/
/*-----FUNCTION PROTOTYPES-----*/
/*-----------------------------*/
// Global functions
void 			MAIN_DispHelp(void);
unsigned char 	MAIN_CheckIP(char *ip);
unsigned char 	MAIN_CheckFileExist(char *path);
void 			MAIN_FileDiv(unsigned long int filesize, unsigned long int *fsize, unsigned long int *fpoint, unsigned char ucPort);

// Sender Main functions - in mainSender.c
unsigned char SENDER_Main(void);
char* SENDER_GetFileName(char *path);
unsigned long int SENDER_GetFileSize(char *path);

// Sender functions - in rftu_sender.c
void* SENDER_Start(void* arg);
void SENDER_AddAllPacket(struct g_stWindows *stWindows, unsigned char N, int file_fd, unsigned int *seq);
void SENDER_AddPacket(struct g_stWindows *stWindows, unsigned char N, int file_fd, unsigned int *seq, int index_finded);
void SENDER_SendPacket(struct g_stWindows *stWindows, unsigned char N, int socket_fd, struct sockaddr_in *si_other, unsigned char all, char cThreadID);
void SENDER_SetAckFlag(struct g_stWindows *stWindows, unsigned char N, unsigned int seq);
int SENDER_FindPacketSeq(struct g_stWindows *stWindows, unsigned char N, unsigned int seq);

// Receiver Main functions - in mainReceiver.c
unsigned char RECEIVER_Main(void);

// Receiver functions - in rftu_receiver.c
void* RECEIVER_Start(void* arg);
int RECEIVER_IsSeqExistInBuffer(struct g_stRFTUPacketData *stRFTUPacketDataBuffer, unsigned int BUFFER_SIZE, unsigned int seq, unsigned int *unRecvBufferSize);
void RECEIVER_InsertPacket(struct g_stRFTUPacketData *rcv_buffer, struct g_stRFTUPacketData rftu_pkt_rcv, unsigned int *unRecvBufferSize);
int RECEIVER_IsFullBuffer(unsigned int *unRecvBufferSize);
void RECEIVER_ResetBuffer(struct g_stRFTUPacketData *stRFTUPacketDataBuffer, unsigned int *unRecvBufferSize);
int RECEIVER_IsEmptyBuffer(unsigned int unRecvBufferSize);

#endif
