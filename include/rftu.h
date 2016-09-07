/*
* Filename: rftu.h
* Author: OCTO team (Issac, Kevin, Peter, Richard)
* Date: 06-Sep-2016
*/
/*************************************************************************/
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

// RFTU Global Variables
extern char 				rftu_filename[256];
extern unsigned long int  	rftu_filesize;
extern char 				rftu_ip[20];
extern unsigned short 		rftu_id;


/*-----------------------------*/
/*-----FUNCTION PROTOTYPES-----*/
/*-----------------------------*/
// Global functions
void 			disp_help(void);
unsigned char 	check_ip(char *ip);
unsigned char 	check_file_exist(char *path);

// Sender functions - in rftu_sender.c
unsigned char 		rftu_sender(void);
char*				get_filename(char *path);
unsigned long int  	get_filesize(char *path);
void 				add_packages(struct windows_t *windows, unsigned char N, int file_fd, unsigned int *seq);
void 				remove_package(struct windows_t *windows, unsigned char N, unsigned int seq);
void 				send_packages(struct windows_t *windows, unsigned char N, int socket_fd, struct sockaddr_in *si_other, unsigned char all);

// Receiver functions - in rftu_receiver.c
unsigned char 	rftu_receiver(void);

#endif
