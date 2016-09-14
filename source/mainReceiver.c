/*
 * Filename: mainReceiver.c
 * Author: OCTO team (Kevin)
 * Date: 13-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

char path[255] = "/";
struct sockaddr_in sender_soc, receiver_soc;
int sd, fd; // socket descriptor and file descriptor

struct file_info_t file_info; // File transfered
struct rftu_packet_cmd_t  rftu_pkt_send_cmd;
struct rftu_packet_data_t rftu_pkt_rcv;

struct timeval timeout; // set time out
fd_set set;
int waiting;
unsigned short rftu_id;
unsigned long int   rftu_filesize;
unsigned long int *fsize;

pthread_t pth[2];
int thread_index;
struct g_stReceiverParam stReceiverParam[2];

unsigned char RECEIVER_Main(void)
{
    unsigned char error_cnt = 0;
    int sd, fd1, fd2; // socket descriptor and file descriptor
    socklen_t socklen = 0;

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

                        // Divide original file to part 
                        fsize = (unsigned long int *)malloc(2*sizeof(unsigned long int));
                        MAIN_div_file(rftu_filesize, fsize);

                        // Create the file to save
                        strcpy(path,"/home/");
                        strcat(path, getlogin());
                        strcat(path, "/Desktop/");
                        strcat(path, file_info.filename);
                        
                        fd1 = open(path, O_CREAT | O_WRONLY, 0666);
                        fd2 = open(path, O_CREAT | O_WRONLY, 0666);
                        if (fd1 < 0 && fd2 < 0)
                        {
                            printf("[RECEIVER Main] There is nospace, cannot create the file\n");
                            // Send command NOSPACE to sender
                            rftu_pkt_send_cmd.cmd = RFTU_CMD_NOSPACE;
                            sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 , (struct sockaddr *) &sender_soc, socklen);
                        }
                        else
                        {
                            printf("[RECEIVER Main] Saving file to : %s\n", path);
                            // Send command READY to sender
                            rftu_pkt_send_cmd.cmd = RFTU_CMD_READY;
                            rftu_pkt_send_cmd.id = rand();
                            rftu_id = rftu_pkt_send_cmd.id;
                            printf("READY cmd.id: %d\n", rftu_id );
                            sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 , (struct sockaddr *) &sender_soc, socklen);

                            // Param for thread 1
                            stReceiverParam[0].nPortNumber = RFTU_PORT_1;
                            stReceiverParam[0].fd = fd1;
                            stReceiverParam[0].nFilePointerStart = 0;
                            stReceiverParam[0].nFileSize = *(fsize + 0);
                            stReceiverParam[0].cThreadID = 0;

                            // Param for thread 2
                            stReceiverParam[1].nPortNumber = RFTU_PORT_2;
                            stReceiverParam[1].fd = fd2;
                            stReceiverParam[1].nFilePointerStart = *(fsize + 0);
                            stReceiverParam[1].nFileSize = *(fsize + 1);
                            stReceiverParam[1].cThreadID = 1;

                            // Thread creation
                            {
                                int m, n;
                                // m = pthread_create(&pth, NULL, &RECEIVER_Start, (void*)&stReceiverParam);

                                m = pthread_create(&pth[0], NULL, &RECEIVER_Start, (void*)&stReceiverParam[0]);
                                n = pthread_create(&pth[1], NULL, &RECEIVER_Start, (void*)&stReceiverParam[1]);

                                if (!m && !n) {
                                    if(flag_verbose == YES)
                                    {
                                        printf("[RECEIVER Main] Thread Created.\n");
                                    }
                                    pthread_join(pth[0], NULL);
                                    pthread_join(pth[1], NULL);
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
                            close(fd1);
                            close(fd2);
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
