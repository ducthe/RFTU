/*
 * Filename: mainReceiver.c
 * Author: OCTO team (Kevin)
 * Date: 13-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

char path[255] = "/";
struct sockaddr_in sender_soc, receiver_soc;

struct g_stFileInfo stFileInfo; // File transfered
struct g_stRFTUPacketData stRFTUPacketDataSend;
struct g_stRFTUPacketData stRFTUPacketDataReceive;
struct g_stPortInfo stPortInfo;

struct timeval stTimeOut; // set time out
fd_set set;
int nWaiting;
unsigned short usRFTUid;
unsigned long int ulRFTUFileSize;
unsigned long int *ulFPoint;
unsigned long int *ulFSize;

pthread_t pth[8];
struct g_stReceiverParam stReceiverParam[8];

unsigned char RECEIVER_Main(void)
{
    const unsigned int RFTU_PORT[THREAD_NUMBER] = {8880, 8881, 8882, 8883, 8884, 8885, 8886, 8887};
    unsigned char ucErrorCnt = 0;
    int nSocketFD; // socket file descriptor
    int nFileDescriptor[THREAD_NUMBER];
    socklen_t socklen = 0;
    int i;

    nSocketFD = socket(PF_INET, SOCK_DGRAM, 0); // socket DGRAM
    if(nSocketFD == -1)
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
    if (bind(nSocketFD, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0)
    {
        printf("[RECEIVER Main] Not binded\n");
        return;
    }
    // printf("[RFTU] Verbose Mode: %s\n", (ucFlagVerbose   == YES ? "ON" : "OFF"));
    printf("%s\n\n", "[RECEIVER Main] Initializing receiver");

    while(1)
    {
        stTimeOut.tv_sec = RFTU_TIMEOUT;
        stTimeOut.tv_usec = 0;
        // Wait Packet
        FD_ZERO (&set);
        FD_SET (nSocketFD, &set);
        nWaiting = select(FD_SETSIZE, &set, NULL, NULL, &stTimeOut);
        switch(nWaiting)
        {
            case 0:  // time out
                ucErrorCnt++;
                if (ucErrorCnt == RFTU_MAX_RETRY)
                {
                    printf("[RECEIVER Main] ERROR: Connection Error\n\n");
                    printf("[RECEIVER Main] Waiting for next files...\n");
                    ucErrorCnt = 0;
                }
                break;
            case -1: // An error occured
                printf("[RECEIVER Main] ERROR: An error occured when nWaiting for INIT packet\n");
                printf("[RECEIVER Main] ERROR: %s", strerror(errno));
                break;
            default: // Read new packet
                recvfrom(nSocketFD, &stRFTUPacketDataReceive, sizeof(stRFTUPacketDataReceive), 0, (struct sockaddr *)&sender_soc, &socklen);
                // check the commanders
                switch(stRFTUPacketDataReceive.ucCmd)
                {
                    case RFTU_CMD_INIT:
                        // Open file
                        stFileInfo = *((struct g_stFileInfo *) &stRFTUPacketDataReceive.ucData);
                        printf("[RECEIVER Main] File info:\n File name : %s, Filesize: %ld bytes \n", stFileInfo.cFileName, stFileInfo.ulFileSize);
                        ulRFTUFileSize = stFileInfo.ulFileSize;

                        // Divide original file to parts
                        ulFSize = (unsigned long int *)malloc(8*sizeof(unsigned long int));
                        ulFPoint = (unsigned long int *)malloc(8*sizeof(unsigned long int));
                        MAIN_div_file(ulRFTUFileSize, ulFSize, ulFPoint);

                        // Create the file to save
                        strcpy(path,"/home/");
                        strcat(path, getlogin());
                        strcat(path, "/Desktop/");
                        strcat(path, stFileInfo.cFileName);

                        for (i = 0; i < THREAD_NUMBER;  i++)
                        {
                            nFileDescriptor[i]= open(path, O_CREAT | O_WRONLY, 0666);
                        }
                        if (nFileDescriptor[0] < 0 && nFileDescriptor[1] < 0 && nFileDescriptor[2] < 0 && nFileDescriptor[3] < 0 && nFileDescriptor[4] < 0 && nFileDescriptor[5] < 0 && nFileDescriptor[6] < 0 && nFileDescriptor[7] < 0)
                        {
                            printf("[RECEIVER Main] There is nospace, cannot create the file\n");
                            // Send command NOSPACE to sender
                            stRFTUPacketDataSend.ucCmd = RFTU_CMD_NOSPACE;
                            sendto(nSocketFD, &stRFTUPacketDataSend, sizeof(stRFTUPacketDataSend) , 0 , (struct sockaddr *) &sender_soc, socklen);
                        }
                        else
                        {
                            printf("[RECEIVER Main] Saving file to : %s\n", path);
                            // Send command READY to sender
                            stRFTUPacketDataSend.ucCmd = RFTU_CMD_READY;
                            stRFTUPacketDataSend.ucID = rand();
                            usRFTUid = stRFTUPacketDataSend.ucID;

                            stPortInfo.ucNumberOfPort = THREAD_NUMBER;
                            for(i = 0; i < THREAD_NUMBER; i++)
                            {
                                stPortInfo.nPortNumber[i] = RFTU_PORT[i];
                            }

                            memcpy(stRFTUPacketDataSend.ucData, &stPortInfo, sizeof(stPortInfo));

                            printf("READY cmd.id: %d\n", usRFTUid );
                            sendto(nSocketFD, &stRFTUPacketDataSend, sizeof(stRFTUPacketDataSend) , 0 , (struct sockaddr *) &sender_soc, socklen);

                            // Parameters for threads
                            for (i = 0; i < THREAD_NUMBER; i++)
                            {
                                stReceiverParam[i].nPortNumber = RFTU_PORT[i];
                                stReceiverParam[i].fd = nFileDescriptor[i];
                                stReceiverParam[i].nFilePointerStart = *(ulFPoint + i);
                                stReceiverParam[i].nFileSize = *(ulFSize + i);
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
                                    if(ucFlagVerbose == YES)
                                    {
                                        printf("[RECEIVER Main] Thread Created.\n");
                                    }
                                    for (i = 0; i < THREAD_NUMBER; i++)
                                    {
                                        pthread_join(pth[i], NULL);
                                    }
                                    if(ucFlagVerbose == YES)
                                    {
                                        printf("[RECEIVER Main] Thread function are terminated.\n");
                                    }
                                }
                                else
                                {
                                    if(ucFlagVerbose == YES)
                                    {
                                        printf("[RECEIVER Main] ERROR: Thread creation failed.\n");
                                    }
                                }
                            }
                            for (i = 0; i < THREAD_NUMBER; i++)
                            {
                                close(nFileDescriptor[i]);
                            }
                            printf("[RECEIVER Main] Waiting for next files...\n");
                        }
                        break;
                    default:
                        printf("[RECEIVER Main] Unknown command %u\n", stRFTUPacketDataReceive.ucCmd);
                        break;
                }
        }
    }
    close(nSocketFD);
    return RFTU_RET_OK;
}
