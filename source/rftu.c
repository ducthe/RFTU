/*
* Filename: main.c
* Author: Henry
* Contributor: Peter + Richard
* Date: 01-Sep-2016
* 
*


*/
/*************************************************************************/
/* Include file */
#include "rftu.h"
/*Defined*/

// RFTU CONTROL FLAGS
unsigned char flag_server  = NO;
unsigned char flag_verbose = NO;
unsigned char flag_file_ok = NO;
unsigned char flag_ip_ok   = NO;

/*Global variables*/
char 				rftu_filename[256];
char 				rftu_ip[20];
unsigned long int  	rftu_filesize = 0;
unsigned short 		rftu_id       = 0;
/*Globale functions*/

int main(int argc, char *argv[])
{
	// Check args
	int opt = 0;

	// if having options, check them and set flags
	while ((opt = getopt(argc, argv, "f:t:svh")) != -1)
	{
		switch (opt)
		{
		case 'f':
			// clear field
			memset(rftu_filename, '\0', sizeof(rftu_filename));
			strcpy(rftu_filename, optarg);

			// check file exist
			if (check_file_exist(rftu_filename) == RFTU_RET_OK)
			{
				flag_file_ok = YES;
			}
			else
			{
				printf("ERROR: File does not exist!!!\nPlease check the path and filename: %s\n\n", rftu_filename);
				disp_help();
				return RFTU_RET_ERROR;
			}
			break;

		case 't':
			// clear field
			memset(rftu_ip, '\0', sizeof(rftu_ip));
			strcpy(rftu_ip, optarg);

			// check IP format
			if (check_ip(rftu_ip) == RFTU_RET_OK)
			{
				flag_ip_ok = YES;
			}
			else
			{
				printf("ERROR: Wrong IP Format!!!\nPlease check the IP: %s\n\n", rftu_ip);
				disp_help();
				return RFTU_RET_ERROR;
			}
			break;
		case 's':
			flag_server = YES;
			break;

		case 'v':
			flag_verbose = YES;
			break;

		case 'h':
			if (flag_file_ok != YES || flag_ip_ok != YES)
			{
				disp_help();
				return RFTU_RET_ERROR;
			}
			break;

		default:
			printf("wrong option\n");
			disp_help();
			return RFTU_RET_ERROR;
		}
	}


	if (flag_file_ok == YES && flag_ip_ok == YES)
	{
		printf("[RFTU] Verbose Mode: %s\n", (flag_verbose   == YES ? "ON" : "OFF"));
		printf("[RFTU] Destination IP: %s\n", rftu_ip);
		printf("[RFTU] Receiver started!!!\n\n");
		rftu_sender();
	}

	if (flag_server == YES)
	{
		printf("[RFTU] Verbose Mode: %s\n", (flag_verbose   == YES ? "ON" : "OFF"));
		printf("[RFTU] Receiver started!!!\n\n");
		rftu_receiver();

	}

	// if file or IP is not OK, show help and quit
	if (flag_file_ok != YES || flag_ip_ok != YES)
	{
		disp_help();
		return RFTU_RET_ERROR;
	}

	// just exit when nothing need to do
	return RFTU_RET_OK;
}

/**
 * [disp_help description]
 */
void disp_help()
{
	printf("Realiable File Transfer over UDP - RFTU 1.0\n\
-------------------------------------------\n\
rftu [-f /path/filename -t destination] [-s] [-v] [-h]\n\n\
  -s: Run as server mode. Files will be overwriten if existed\n\n\
  -f: path to file that will be sent, must have -t option\n\
  -t: IP of destination address, in format X.X.X.X, in pair with -f option\n\n\
  -v: show progress information, this will slow down the progress\n\n\
  -h: show help information\n\n");
}

/**
 * [check_file_exist description]
 * @param  path [description]
 * @return      [description]
 */
unsigned char check_file_exist(char *path)
{
	if ( access( path, F_OK | R_OK) != -1 )
	{
		// file exist
		return RFTU_RET_OK;
	}
	else
	{
		// file doesn't exist
		return RFTU_RET_ERROR;
	}
}

/**
 * [check_ip description]
 * @param  ip [description]
 * @return    [description]
 */
unsigned char check_ip(char *ip)
{
	int ele, oct1, oct2, oct3, oct4;
	ele = sscanf(ip, "%d.%d.%d.%d", &oct1, &oct2, &oct3, &oct4);
	if (ele == 4)
	{
		if ( oct1 >= 0 && oct1 <= 255 &&
		        oct2 >= 0 && oct2 <= 255 &&
		        oct2 >= 0 && oct3 <= 255 &&
		        oct4 >= 0 && oct4 <= 255)
			return RFTU_RET_OK;
		else
			return RFTU_RET_ERROR;
	}
	else
		return RFTU_RET_ERROR;
}

/**
 * [socket_error description]
 * @param socket_fd [description]
 * @param message   [description]
 */
void socket_error(int socket_fd, char *message)
{
	printf("%s\n", message);
	close(socket_fd);
}

/**
 * [rftu_delay description]
 * @param n      [description]
 * @param info   [description]
 * @param unused [description]
 */
void rftu_delay(int n, siginfo_t *info, void *unused)
{
	static int interrupt = 0;
	interrupt ++;
	printf("[RFTU] interrupted %i time(s)\n", interrupt);
	sleep(1);

	if (interrupt == 3)
	{
		printf("[RFTU] Quit now\n");
		exit(1);
	}
}
