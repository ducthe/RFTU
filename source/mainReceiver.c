/*
 * Filename: mainReceiver.c
 * Author: OCTO team (Kevin)
 * Date: 13-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

char path[255] = "/";
struct sockaddr_in sender_soc, receiver_soc;

struct file_info_t file_info; // File transfered
// struct rftu_packet_cmd_t  rftu_pkt_send_init;
struct rftu_packet_data_t rftu_pkt_send_init;
struct rftu_packet_data_t rftu_pkt_rcv;
struct g_stPortInfo port_info;

struct timeval timeout; // set time out
fd_set set;
int waiting;
unsigned short rftu_id;
unsigned long int   rftu_filesize;
unsigned long int *fpoint;
unsigned long int *fsize;

pthread_t pth[8];
struct g_stReceiverParam stReceiverParam[8];

unsigned char RECEIVER_Main(void)
{
    const unsigned int RFTU_PORT[THREAD_NUMBER] = {8880, 8881, 8882, 8883, 8884, 8885, 8886, 8887};
    unsigned char error_cnt = 0;
    int sd; // socket descriptor
    int fd[THREAD_NUMBER];
    socklen_t socklen = 0;
    int i;

    sd = socket(PF_INET, SOCK_DGRAM, 0); // socket DGRAM
    if(sd == -1)
    {
        printf("[RECEIVER Main] ERROR: Fail to create socket\n");
        return;
    }

    // Init socket structure
    memset((char *) &receiver_soc, 0, sizeof(receiver_soc));
    receiver_soc.sin_family      = AF_INET;
    receiver_soc.sin_port        = htons(RFTU_WELCOME_PORT);
    receiver_soc.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(sender_soc);

    // Create a socket for receiver
    if (bind(sd, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0)
    {
        printf("[RECEIVER Main] Not binded\n");
        return;
    }
    // printf("[RFTU] Verbose Mode: %s\n", (flag_verbose   == YES ? "ON" : "OFF"));
    printf("%s\n\n", "[RECEIVER Main] Initializing receiver");

    while(1)
    {
        timeout.tv_sec = RFTU_TIMEOUT;
        timeout.tv_usec = 0;
        // Wait Packet
        FD_ZERO (&set);
        FD_SET (sd, &set);
        waiting = select(FD_SETSIZE, &set, NULL, NULL,
                &timeout);
        switch(waiting)
        {
            case 0:  // time out
                error_cnt++;
                if (error_cnt == RFTU_MAX_RETRY)
                {
                    printf("[RECEIVER Main] ERROR: Connection Error\n\n");
                    printf("[RECEIVER Main] Waiting for next files...\n");
                    error_cnt = 0;
                }
                break;
            case -1: // An error occured
                printf("[RECEIVER Main] ERROR: An error occured when waiting for INIT packet\n");
                printf("[RECEIVER Main] ERROR: %s", strerror(errno));
                break;
            default: // Read new packet
                recvfrom(sd, &rftu_pkt_rcv, sizeof(rftu_pkt_rcv), 0, (struct sockaddr *)&sender_soc, &socklen);
                // check the commanders
                switch(rftu_pkt_rcv.cmd)
                {
                    case RFTU_CMD_INIT:
                        // Open file
                        file_info = *((struct file_info_t *) &rftu_pkt_rcv.data);
                        printf("[RECEIVER Main] File info:\n File name : %s, Filesize: %ld bytes \n", file_info.filename, file_info.filesize);
                        rftu_filesize = file_info.filesize;

                        // Divide original file to parts
                        fsize = (unsigned long int *)malloc(8*sizeof(unsigned long int));
                        fpoint = (unsigned long int *)malloc(8*sizeof(unsigned long int));
                        MAIN_div_file(rftu_filesize, fsize, fpoint);

                        // Create the file to save
                        strcpy(path,"/home/");
                        strcat(path, getlogin());
                        strcat(path, "/Desktop/");
                        strcat(path, file_info.filename);

                        for (i = 0; i < THREAD_NUMBER;  i++)
                        {
                            fd[i]= open(path, O_CREAT | O_WRONLY, 0666);
                        }
                        if (fd[0] < 0 && fd[1] < 0 && fd[2] < 0 && fd[3] < 0 && fd[4] < 0 && fd[5] < 0 && fd[6] < 0 && fd[7] < 0)
                        {
                            printf("[RECEIVER Main] There is nospace, cannot create the file\n");
                            // Send command NOSPACE to sender
                            rftu_pkt_send_init.cmd = RFTU_CMD_NOSPACE;
                            sendto(sd, &rftu_pkt_send_init, sizeof(rftu_pkt_send_init) , 0 , (struct sockaddr *) &sender_soc, socklen);
                        }
                        else
                        {
                            printf("[RECEIVER Main] Saving file to : %s\n", path);
                            // Send command READY to sender
                            rftu_pkt_send_init.cmd = RFTU_CMD_READY;
                            rftu_pkt_send_init.id = rand();
                            rftu_id = rftu_pkt_send_init.id;

                            port_info.ucNumberOfPort = THREAD_NUMBER;
                            for(i = 0; i < THREAD_NUMBER; i++)
                            {
                                port_info.nPortNumber[i] = RFTU_PORT[i];
                            }

                            memcpy(rftu_pkt_send_init.data, &port_info, sizeof(port_info));

                            printf("READY cmd.id: %d\n", rftu_id );
                            sendto(sd, &rftu_pkt_send_init, sizeof(rftu_pkt_send_init) , 0 , (struct sockaddr *) &sender_soc, socklen);

                            // Parameters for threads
                            for (i = 0; i < THREAD_NUMBER; i++)
                            {
                                stReceiverParam[i].nPortNumber = RFTU_PORT[i];
                                stReceiverParam[i].fd = fd[i];
                                stReceiverParam[i].nFilePointerStart = *(fpoint + i);
                                stReceiverParam[i].nFileSize = *(fsize + i);
                                stReceiverParam[i].cThreadID = i;
                            }

                            // Thread creation
                            {
                                int m[8];
                                for (i = 0; i < THREAD_NUMBER; i++)
                                {
                                    m[i] = pthread_create(&pth[i], NULL, &RECEIVER_Start, (void*)&stReceiverParam[i]);
                                }

                                if (!m[0] && !m[1] && !m[2] && !m[3] && !m[4] && !m[5] && !m[6] && !m[7])
                                {
                                    if(flag_verbose == YES)
                                    {
                                        printf("[RECEIVER Main] Thread Created.\n");
                                    }
                                    for (i = 0; i < THREAD_NUMBER; i++)
                                    {
                                        pthread_join(pth[i], NULL);
                                    }
                                    if(flag_verbose == YES)
                                    {
                                        printf("[RECEIVER Main] Thread function are terminated.\n");
                                    }
                                }
                                else
                                {
                                    if(flag_verbose == YES)
                                    {
                                        printf("[RECEIVER Main] ERROR: Thread creation failed.\n");
                                    }
                                }
                            }
                            for (i = 0; i < THREAD_NUMBER; i++)
                            {
                                close(fd[i]);
                            }
                            printf("[RECEIVER Main] Waiting for next files...\n");
                        }
                        break;
                    default:
                        printf("[RECEIVER Main] Unknown command %u\n", rftu_pkt_rcv.cmd);
                        break;
                }
        }
    }
    close(sd);
    return RFTU_RET_OK;
}
