/*
* Filename: rftu.h
* Author: OCTO team (Issac, Kevin, Peter, Richard)
* Date: 06-Sep-2016
*/
/*************************************************************************/
#ifndef __RFTU_H__
#define __RFTU_H__
#define DROPPER

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

/*-------------------*/
/*------MACROS-------*/
/*-------------------*/
//#define DROPPER

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
#define RFTU_FRAME_SIZE     1024
#define RFTU_PORT 			8888
#define RFTU_TIMEOUT		3
#define RFTU_MAX_RETRY		10
#define RFTU_WINDOW_SIZE	8

// RFTU Control Flags
extern unsigned char flag_server;
extern unsigned char flag_verbose;
extern unsigned char flag_file_ok;
extern unsigned char flag_ip_ok;

// RFTU Data Package
struct rftu_packet_data_t {
	unsigned char 	cmd;						/* if cmd = DATA or INIT, size and data is available */
	unsigned char 	id;
	unsigned int  	seq;
	unsigned short 	size;
	unsigned char 	data[RFTU_FRAME_SIZE];
};

// RFTU Command Package
struct rftu_packet_cmd_t {
	unsigned char 	cmd;
	unsigned char 	id;
	unsigned int  	seq;
	unsigned short 	size;
};

// RFTU File Info
struct file_info_t {
	char 				filename[256];
	unsigned long int  	filesize;
};

// RFTU window
struct windows_t {
	unsigned char sent;
	unsigned char ack;
	struct rftu_packet_data_t package;
};

struct senderParam {
    int portNumber;
};

// RFTU Global Variables
extern char 				rftu_filename[256];
extern unsigned long int  	rftu_filesize;
extern char 				rftu_ip[20];
extern unsigned short 		rftu_id;


/*-----------------------------*/
/*-----FUNCTION PROTOTYPES-----*/
/*-----------------------------*/
// Global functions
void 			MAIN_disp_help(void);
unsigned char 	MAIN_check_ip(char *ip);
unsigned char 	MAIN_check_file_exist(char *path);

// Sender functions - in rftu_sender.c
void* SENDER_Start(void* arg);
char* SENDER_Get_Filename(char *path);
unsigned long int SENDER_Get_Filesize(char *path);
void SENDER_AddAllPackages(struct windows_t *windows, unsigned char N, int file_fd, unsigned int *seq);
void SENDER_Add_Package(struct windows_t *windows, unsigned char N, int file_fd, unsigned int *seq, int index_finded);
void SENDER_Send_Packages(struct windows_t *windows, unsigned char N, int socket_fd, struct sockaddr_in *si_other, unsigned char all);
void SENDER_SetACKflag(struct windows_t *windows, unsigned char N, unsigned int seq);
int SENDER_FindPacketseq(struct windows_t *windows, unsigned char N, unsigned int seq);

// Receiver functions - in rftu_receiver.c
unsigned char 	RECEIVER_Start(void);
int RECEIVER_isSeqExistInBuffer(struct rftu_packet_data_t *rcv_buffer, unsigned int BUFFER_SIZE, unsigned int seq);
void RECEIVER_InsertPacket(struct rftu_packet_data_t *rcv_buffer, struct rftu_packet_data_t rftu_pkt_rcv);
void RECEIVER_RemovePacket(struct rftu_packet_data_t *rcv_buffer, int BUFFER_SIZE, struct rftu_packet_data_t rftu_pkt_rcv);
int RECEIVER_IsFullBuffer();
void RECEIVER_ResetBuffer(struct rftu_packet_data_t *rcv_buffer);
int RECEIVER_IsEmptyBuffer();

#endif
