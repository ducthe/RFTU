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

pthread_t pth;
struct g_stReceiverParam stReceiverParam;

unsigned char RECEIVER_Main(void)
{
    unsigned char error_cnt = 0;
    socklen_t socklen = 0;

    sd = socket(PF_INET, SOCK_DGRAM, 0); // socket DGRAM
    if(sd == -1)
    {
        printf("[RECEIVER] ERROR: Fail to create socket\n");
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
        printf("[RECEIVER] Not binded\n");
        return;
    }
    printf("%s\n\n", "[RECEIVER] Initializing receiver");

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
                    printf("[RECEIVER] ERROR: Connection Error\n\n");
                    printf("[RECEIVER] Waiting for next files...\n");
                    error_cnt = 0;
                }
                break;
            case -1: // An error occured
                printf("[RECEIVER] ERROR: An error occured when waiting for INIT packet\n");
                break;
            default: // Read new packet
                recvfrom(sd, &rftu_pkt_rcv, sizeof(rftu_pkt_rcv), 0, (struct sockaddr *)&sender_soc, &socklen);
                // check the commanders
                switch(rftu_pkt_rcv.cmd)
                {
                    case RFTU_CMD_INIT:
                        // Open file
                        file_info = *((struct file_info_t *) &rftu_pkt_rcv.data);
                        printf("File info:\n File name : %s, Filesize: %ld bytes \n", file_info.filename, file_info.filesize);
                        rftu_filesize = file_info.filesize;
                        // Create the file to save
                        strcpy(path,"/home/");
                        strcat(path, getlogin());
                        strcat(path, "/Desktop/");
                        strcat(path, file_info.filename);
                        fd = open(path, O_CREAT | O_WRONLY, 0666);
                        if (fd < 0)
                        {
                            printf("[RECEIVER] There is nospace, cannot create the file\n");
                            // Send command NOSPACE to sender
                            rftu_pkt_send_cmd.cmd = RFTU_CMD_NOSPACE;
                            sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 , (struct sockaddr *) &sender_soc, socklen);
                        }
                        else
                        {
                            printf("[RECEIVER] Saving file to : %s\n", path);
                            // Send command READY to sender
                            rftu_pkt_send_cmd.cmd = RFTU_CMD_READY;
                            rftu_pkt_send_cmd.id = rand();
                            rftu_id = rftu_pkt_send_cmd.id;
                            printf("READY cmd.id: %d\n", rftu_id );
                            sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 , (struct sockaddr *) &sender_soc, socklen);

                            stReceiverParam.nPortNumber = RFTU_PORT_1;
                            stReceiverParam.fd = fd;
                            stReceiverParam.nFileSize = rftu_filesize;

                            // Thread creation
                            {
                                int m;
                                m = pthread_create(&pth, NULL, &RECEIVER_Start, (void*)&stReceiverParam);
                                if (!m) {
                                    printf("[RECEIVER] Thread Created.\n");
                                    pthread_join(pth, NULL);
                                    printf("[RECEIVER] Thread function terminated.\n");
                                }
                                else
                                    printf("[RECEIVER] ERROR: Thread creation failed.\n");
                            }
                            close(fd);
                            printf("[RECEIVER] Waiting for next files...\n");
                        }
                        break;
                    default:
                        printf("Unknown command %u\n", rftu_pkt_rcv.cmd);
                        break;
                }
        }
    }
    close(sd);
    return RFTU_RET_OK;
}