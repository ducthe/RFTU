/*
*   Filename    : mainReceiver.c
*   Author      : Kevin (OCTO Team)
*   Date        : 13-Sep-2016
*/

#include "rftu.h"

char path[255] = "/";
struct sockaddr_in stSenderAddr, stReceiverAddr;

struct g_stFileInfo stFileInfo; /* File transfered */
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

pthread_t *pth;
struct g_stReceiverParam *stReceiverParam;

unsigned char RECEIVER_Main(void)
{
    const unsigned int RFTU_PORT[THREAD_NUMBER] = {8880, 8881, 8882, 8883, 8884, 8885, 8886, 8887};
    unsigned char ucErrorCnt = 0;
    int nSocketFD;  /* Socket file descriptor */
    int *nFileDescriptor;
    socklen_t socklen = 0;
    int i;

    nSocketFD = socket(PF_INET, SOCK_DGRAM, 0); /* DGRAM Socket */
    if(nSocketFD == -1)
    {
        printf("[RECEIVER Main] ERROR: Fail to create socket\n");
        return;
    }

    /* Init socket structure */
    memset((char *) &stReceiverAddr, 0, sizeof(stReceiverAddr));
    stReceiverAddr.sin_family      = AF_INET;
    stReceiverAddr.sin_port        = htons(RFTU_WELCOME_PORT);
    stReceiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(stSenderAddr);

    /* Create a socket for receiver */
    if (bind(nSocketFD, (struct sockaddr *)&stReceiverAddr, sizeof(stReceiverAddr)) != 0)
    {
        printf("[RECEIVER Main] Not binded\n");
        return;
    }
    printf("%s\n\n", "[RECEIVER Main] Initializing receiver");

    while(1)
    {
        stTimeOut.tv_sec = RFTU_TIMEOUT;
        stTimeOut.tv_usec = 0;
        /* Wait for Packet */
        FD_ZERO (&set);
        FD_SET (nSocketFD, &set);
        nWaiting = select(FD_SETSIZE, &set, NULL, NULL, &stTimeOut);
        switch(nWaiting)
        {
            case 0:  /* Time out */
                ucErrorCnt++;
                if (ucErrorCnt == RFTU_MAX_RETRY)
                {
                    printf("[RECEIVER Main] ERROR: Connection Error\n\n");
                    printf("[RECEIVER Main] Waiting for next files...\n");
                    ucErrorCnt = 0;
                }
                break;
            case -1: /* Error */
                printf("[RECEIVER Main] ERROR: An error occured when nWaiting for INIT packet\n");
                printf("[RECEIVER Main] ERROR: %s", strerror(errno));
                break;
            default: /* Read received packet */
                recvfrom(nSocketFD, &stRFTUPacketDataReceive, sizeof(stRFTUPacketDataReceive), 0, (struct sockaddr *)&stSenderAddr, &socklen);
                /* Check commands */
                switch(stRFTUPacketDataReceive.ucCmd)
                {
                    case RFTU_CMD_INIT:
                        /* Open File */
                        stFileInfo = *((struct g_stFileInfo *) &stRFTUPacketDataReceive.ucData);
                        printf("[RECEIVER Main] FILE INFO:\n    File name: %s\n    File size: %ld bytes\n", stFileInfo.cFileName, stFileInfo.ulFileSize);
                        ulRFTUFileSize = stFileInfo.ulFileSize;

                        /* Create the file to save */
                        strcpy(path,"/home/");
                        strcat(path, getlogin());
                        strcat(path, "/Desktop/");
                        strcat(path, stFileInfo.cFileName);

                        nFileDescriptor = (int *) malloc(unThreadNumber * sizeof(int));
                        for (i = 0; i < unThreadNumber;  i++)
                        {
                            *(nFileDescriptor + i) = open(path, O_CREAT | O_WRONLY, 0666);
                            if (*(nFileDescriptor + i) == -1)
                            {
                                printf("[RECEIVER Main] ERROR: There is not enough space to write.\n");
                                /* Send command NOSPACE to sender */
                                stRFTUPacketDataSend.ucCmd = RFTU_CMD_NOSPACE;
                                sendto(nSocketFD, &stRFTUPacketDataSend, sizeof(stRFTUPacketDataSend) , 0 , (struct sockaddr *) &stSenderAddr, socklen);
                                return RFTU_RET_ERROR;
                            }
                        }
                        /* Divide original file to a number of parts */
                        pth = (pthread_t *)malloc(unThreadNumber * sizeof(pthread_t));
                        stReceiverParam = (struct g_stReceiverParam *)malloc(unThreadNumber * sizeof(struct g_stReceiverParam));
                        ulFSize = (unsigned long int *)malloc(unThreadNumber * sizeof(unsigned long int));
                        ulFPoint = (unsigned long int *)malloc(unThreadNumber * sizeof(unsigned long int));
                        MAIN_FileDiv(ulRFTUFileSize, ulFSize, ulFPoint, unThreadNumber);

                        printf("[RECEIVER Main] Saving file to : %s\n", path);
                        /* Send command READY to sender */
                        stRFTUPacketDataSend.ucCmd = RFTU_CMD_READY;
                        stRFTUPacketDataSend.ucID = rand();
                        usRFTUid = stRFTUPacketDataSend.ucID;

                        stPortInfo.ucNumberOfPort = unThreadNumber;
                        for(i = 0; i < unThreadNumber; i++)
                        {
                            stPortInfo.nPortNumber[i] = RFTU_PORT[i];
                        }

                        memcpy(stRFTUPacketDataSend.ucData, &stPortInfo, sizeof(stPortInfo));

                        printf("[RECEIVER Main] ID of transmission: %d\n", usRFTUid);
                        sendto(nSocketFD, &stRFTUPacketDataSend, sizeof(stRFTUPacketDataSend) , 0 , (struct sockaddr *) &stSenderAddr, socklen);

                        /* Set parameters for threads */
                        for (i = 0; i < unThreadNumber; i++)
                        {
                            (stReceiverParam + i)->nPortNumber = RFTU_PORT[i];
                            (stReceiverParam + i)->fd = *(nFileDescriptor + i);
                            (stReceiverParam + i)->nFilePointerStart = *(ulFPoint + i);
                            (stReceiverParam + i)->nFileSize = *(ulFSize + i);
                            (stReceiverParam + i)->cThreadID = i;
                        }

                        /* Thread creation */
                        {
                            int *m;
                            int a = 0;
                            m = (int*)malloc(unThreadNumber*sizeof(int));
                            for (i = 0; i < unThreadNumber; i++)
                            {
                                *(m+i) = pthread_create(pth + i, NULL, &RECEIVER_Start, (void*)(stReceiverParam + i));
                                a = a | (*(m+i));
                            }

                            if (!a)
                            {
                                if(ucFlagVerbose == YES)
                                {
                                    printf("[RECEIVER Main] Thread Created.\n");
                                }
                                for (i = 0; i < unThreadNumber; i++)
                                {
                                    pthread_join(*(pth + i), NULL);
                                }
                                if(ucFlagVerbose == YES)
                                {
                                    printf("[RECEIVER Main] Thread function are terminated.\n");
                                }
                            }
                            else
                            {
                                if (ucFlagVerbose == YES)
                                {
                                    printf("[RECEIVER Main] ERROR: Thread creation failed.\n");
                                }
                            }
                            free(m);
                        }
                        for (i = 0; i < unThreadNumber; i++)
                        {
                            close(*(nFileDescriptor + i));
                        }
                        free(nFileDescriptor);
                        free(pth);
                        free(stReceiverParam);
                        free(ulFSize);
                        free(ulFPoint);
                        printf("[RECEIVER Main] Waiting for next files...\n");
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
