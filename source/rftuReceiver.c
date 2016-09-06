/**********************************************************************

* Filename : rftuReciever.c
* Author: Peter + Richard
* Date : 01-Sep-2016


***********************************************************************/
// Include 
#include "rftu.h"

// Variables

unsigned char *path = "~/Downloads";

struct sockaddr_in sender_soc, receiver_soc;
int sd, soc_len; // socket descriptor
unsigned char error_cnt = 0;

struct file_info_t file_info; // File transfered
struct rftu_package_cmd_t  rftu_pkg_send_cmd; 
struct rftu_package_data_t rftu_pkg_rcv; 
struct timval timeout; // set time out
char recieving = NO;



int rftu_reciever(void)
{

	sd = socket(AF_INET, SOCK_DGRAM, 0); // socket DGRAM
	if(sd == -1)
	{
		printf("Error to create socket\n");
		return -1;
	}
// init socket structure
	memset((char *) &receiver_soc, 0, sizeof(receiver_soc));
	receiver_soc.sin_family      = AF_INET;
	receiver_soc.sin_port        = htons(RFTU_PORT);
	receiver_soc.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( bind(sd, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0 ) // Create socket for receiver
	{
    	printf("Not binded\n");
	}


	return 0;
}