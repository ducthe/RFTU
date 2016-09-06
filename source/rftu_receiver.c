/**********************************************************************

* Filename : rftu_receiver.c
* Author: Peter + Richard
* Date : 01-Sep-2016


***********************************************************************/
// Include 
#include "rftu.h"

// Variables

unsigned char path[200] = "~/Downloads/";

struct sockaddr_in sender_soc, receiver_soc;
int sd, fd; // socket descriptor and file descriptor


struct file_info_t file_info; // File transfered
struct rftu_package_cmd_t  rftu_pck_send_cmd; 
struct rftu_package_data_t rftu_pck_rcv; 
struct timval timeout; // set time out
char receiving = NO;
fd_set set;
int waiting;
unsigned char error_cnt = 0;
unsigned int Rn;
unsigned long int receiver_bytes;


unsigned char rftu_receiver(void)
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
	if ( bind(sd, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0 ) // Create a socket for receiver
	{
    	printf("Not binded\n");
	}



	while(1)
	{
		//Set time out
		timeout.tv_sec = RFTU_TIMEOUT;
		timeout.tv_usec = 0;
		// Wait Packet
		FD_ZERO (&set);
 		FD_SET (sd, &set);
		waiting = select(FD_SETSIZE, &set, NULL, NULL, 
						 &timeout);
		switch(waiting)
		{
			case 0: // time out
			if(receiving ==  YES)
			{
				error_cnt++;
				break;
			}
			case -1: // have an error
				printf("Error when waiting for package \n");
				break;
			default: // Read new packet
				recvfrom(sd, &rftu_pck_rcv, sizeof(rftu_pck_rcv), 0, 
							(struct sockaddr *)&sender_soc, sizeof(sender_soc));
				// check the commanders
				switch(rftu_pck_rcv.cmd)
				{
					case (RFTU_CMD_INIT):
					if(receiving == NO)
				    {
				    	// Open file
				    	file_info = *((struct file_info_t *) &rftu_pck_rcv.data);
				    	printf("File info:\n File name : %s, Filesize: %ld bytes \n", file_info.filename, file_info.filesize);
				    	// Create the file to save
				    	strcat(path, file_info.filename);
				    	fd = open(path,  O_CREAT | O_WRONLY,  0666);
				    	if (file_fd < 0)
				    	{
				    		printf("There is nospace, cannot create the file\n");
				    		// Send command NOSPACE to sender
				    		rftu_pck_send_cmd.cmd = RFTU_CMD_NOSPACE;
				    		sendto(sd, &rftu_pck_send_cmd, sizeof(rftu_pck_send_cmd) , 0 ,
				    				 (struct sockaddr *) &sender_soc, sizeof(sender_soc));

				    	}
				    	else
				    	{
				    		// Send command READY to sender
				    		rftu_pck_send_cmd.cmd = RFTU_CMD_READY;
				    		rftu_pck_send_cmd.id = rand();
				    		sendto(sd, &rftu_pck_send_cmd, sizeof(rftu_pck_send_cmd) , 0 , 
				    				(struct sockaddr *) &sender_soc, sizeof(sender_soc));

				    		receiving = YES;
				    		receiver_bytes = 0;


				    	}
				    	break;

				    }
				    case (RFTU_CMD_DATA):
				    	if (receiving == YES)
				    	{

				    	}
				    	break;
				    default: 

				    	break;
				}


		}


	}

	return 0;
}