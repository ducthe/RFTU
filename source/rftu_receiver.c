/**********************************************************************

* Filename : rftu_receiver.c
* Author: Peter + Richard
* Date : 01-Sep-2016


***********************************************************************/
// Include 
#include "rftu.h"

// Variables

unsigned char path[250] = "~/Downloads/";

struct sockaddr_in sender_soc, receiver_soc;
int sd, fd; // socket descriptor and file descriptor


struct file_info_t file_info; // File transfered
struct rftu_package_cmd_t  rftu_pck_send_cmd; 
struct rftu_package_data_t rftu_pck_rcv; 
struct timeval timeout; // set time out
char receiving = NO;
fd_set set;
int waiting;
unsigned char error_cnt = 0;
unsigned int Rn;
unsigned short rftu_id;
unsigned long int received_bytes;
unsigned long int   rftu_filesize;
socklen_t socklen;


unsigned char rftu_receiver(void)
{
    printf("%s\n\n", "Initialize receiver");
	sd = socket(AF_INET, SOCK_DGRAM, 0); // socket DGRAM
	if(sd == -1)
	{
		printf("Error to create socket\n");
		return RFTU_RET_ERROR;
	}
// init socket structure
	memset((char *) &receiver_soc, 0, sizeof(receiver_soc));
	receiver_soc.sin_family      = AF_INET;
	receiver_soc.sin_port        = htons(RFTU_PORT);
	receiver_soc.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(sender_soc);

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
    			}
                break;
			case -1: // have an error
				printf("Error when waiting for package \n");
				break;
			default: // Read new packet
				recvfrom(sd, &rftu_pck_rcv, sizeof(rftu_pck_rcv), 0, \
                    (struct sockaddr *)&sender_soc, &socklen);
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
				    	if (fd < 0)
				    	{
				    		printf("There is nospace, cannot create the file\n");
				    		// Send command NOSPACE to sender
				    		rftu_pck_send_cmd.cmd = RFTU_CMD_NOSPACE;
				    		sendto(sd, &rftu_pck_send_cmd, sizeof(rftu_pck_send_cmd) , 0 ,
				    				 (struct sockaddr *) &sender_soc, socklen);

				    	}
				    	else
				    	{
				    		// Send command READY to sender
				    		rftu_pck_send_cmd.cmd = RFTU_CMD_READY;
				    		rftu_pck_send_cmd.id = rand();
				    		rftu_id = rftu_pck_send_cmd.id;
				    		printf("INIT cmd.id: %d\n",rftu_id );
				    		sendto(sd, &rftu_pck_send_cmd, sizeof(rftu_pck_send_cmd) , 0 , 
				    				(struct sockaddr *) &sender_soc, socklen);

				    		receiving = YES;
				    		received_bytes = 0;


				    	}
				    	break;

				    }
				    case (RFTU_CMD_DATA):
				    	if (receiving == YES)
				    	{
				    		if (rftu_pck_rcv.id == rftu_id)
							{
								if (rftu_pck_rcv.seq == Rn) 
									{
										write(fd, rftu_pck_rcv.data, rftu_pck_rcv.size);
										received_bytes += rftu_pck_rcv.size;
										Rn++;
										error_cnt = 0;
										rftu_pck_send_cmd.cmd  = RFTU_CMD_ACK;
										rftu_pck_send_cmd.seq  = Rn;
										rftu_pck_send_cmd.size = (received_bytes * 100 / rftu_filesize);

										sendto(sd, &rftu_pck_send_cmd, sizeof(rftu_pck_send_cmd) , 0 ,
				    							 (struct sockaddr *) &sender_soc, socklen);

										// When received file completly
										if (received_bytes == file_info.filesize)
										{

											printf("[RECEIVER] Sending COMPLETED cmd\n");

											// send complete command
											rftu_pck_send_cmd.cmd  = RFTU_CMD_COMPLETED;
											rftu_pck_send_cmd.id   = rftu_id;
											rftu_pck_send_cmd.seq  = Rn;
											rftu_pck_send_cmd.size = (received_bytes * 100 / rftu_filesize);
											
											sendto(sd, &rftu_pck_send_cmd, sizeof(rftu_pck_send_cmd) , 0 ,
				    								 (struct sockaddr *) &sender_soc, socklen);

											// close file
											close(fd);
											printf("Received file completed\n\n");
											printf(" Waiting... for INIT cmd !\n");

											// dont get any DATA segment
											receiving = NO;
										}

										error_cnt = 0;
									}
								}
								else
									{
										printf("[RECEIVER] Unknown ID: %i\n", rftu_pck_rcv.id);
									}
						}
                    default: 
                        printf("Unknown command %u\n", rftu_pck_rcv.cmd);
                        break;
				}
                break;
				    
		}

		if (receiving == YES)
		{
			if (error_cnt == RFTU_MAX_RETRY)
			{
				printf("Error: Connection\n\n");

				// close file
				close(fd);
				printf(" Waiting... for INIT command\n");
				receiving = NO;

				error_cnt = 0;
			}
		}
	}


	close(sd);
	return RFTU_RET_OK;
}
