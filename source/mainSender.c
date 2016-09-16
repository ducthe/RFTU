/*
*   Filename    : mainSender.c
*   Author      : Kevin (OCTO Team)
*   Date        : 13-Sep-2016
*/

#include "rftu.h"

unsigned char SENDER_Main(void)
{
    pthread_t *pth;
    struct g_stSenderParam *stSenderParam;

    /* Sender variables */
    struct g_stFileInfo stFileInfo;                    /* File info to be sent to receiver in INIT message */
    struct g_stRFTUPacketData stRFTUPacketDataSend;    /* Packet to be sent */
    struct g_stRFTUPacketData stRFTUPacketDataReceive; /* Used to store received package */
    struct g_stPortInfo stPortInfoReceive;             /* Used to store received information about port */

    struct sockaddr_in stReceiverAddr; /* Receiver address */
    socklen_t socklen = 0;
    int nSocketFd;  /* Socket file descriptor */

    unsigned char ucErrorCnt = 0;

    fd_set fds; /* Set of file descriptors to be monitored by select() */
    int nSelectResult;
    struct timeval stTimeOut;

    char *cTmp;
    int i;

    unsigned long int *ulFSize;
    unsigned long int *ulFPoint;

    /* File info setup */
    cTmp = (char*)malloc(sizeof(cRFTUFileName));
    strcpy(cTmp, cRFTUFileName);
    strcpy(stFileInfo.cFileName, SENDER_GetFileName(cTmp));
    free(cTmp);
    stFileInfo.ulFileSize = SENDER_GetFileSize(cRFTUFileName);
    ulRFTUFileSize = stFileInfo.ulFileSize;
    printf("[RECEIVER Main] FILE INFO:\n    File name: %s\n    File size: %ld bytes\n", stFileInfo.cFileName, stFileInfo.ulFileSize);

    /* Configure settings of the receiver address struct */
    stReceiverAddr.sin_family = AF_INET;
    stReceiverAddr.sin_port = htons(RFTU_WELCOME_PORT); /* 8880 */
    if (inet_aton(cRFTUIP, &stReceiverAddr.sin_addr) == 0)
    {
        printf("[SENDER Main] ERROR: The address is invalid\n");
        return RFTU_RET_ERROR;
    }
    memset(stReceiverAddr.sin_zero, '\0', sizeof(stReceiverAddr.sin_zero));
    socklen = sizeof(stReceiverAddr);

    /* Socket creation */
    if ((nSocketFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("[SENDER Main] ERROR: Socket creation fails\n");
        return RFTU_RET_ERROR;
    }

    /* ---START--- */
    ucErrorCnt = 0;
    while(1)
    {
        /* Create INIT packet */
        stRFTUPacketDataSend.ucCmd = RFTU_CMD_INIT;
        stRFTUPacketDataSend.ucID = 0;
        stRFTUPacketDataSend.unSeq = 0;
        stRFTUPacketDataSend.usSize = sizeof(stFileInfo);
        memcpy(stRFTUPacketDataSend.ucData, &stFileInfo, sizeof(stFileInfo));

        /* Sent the INIT packet */
        printf("[SENDER Main] Sending INIT message\n");
        sendto(nSocketFd, &stRFTUPacketDataSend, sizeof(stRFTUPacketDataSend), 0, (struct sockaddr*)&stReceiverAddr, socklen);

        /* Initialize stTimeOut */
        stTimeOut.tv_sec = RFTU_TIMEOUT;
        stTimeOut.tv_usec = 0;

        /* Set up the set of descriptors */
        FD_ZERO(&fds);  /* Let the file descriptor set becomes all zero */
        FD_SET(nSocketFd, &fds);  /* Add the nSocketFd to the set */

        /* Check time out using select() function */
        nSelectResult = select(FD_SETSIZE, &fds, NULL, NULL, &stTimeOut);
        if (nSelectResult == -1) // Error
        {
            printf("[SENDER Main] ERROR: Error while waiting for packets\n");
            printf("[SENDER Main] ERROR: %s", strerror(errno));
            close(nSocketFd);
            return RFTU_RET_ERROR;
        }
        else if (nSelectResult == 0) /* Time out */
        {
            ucErrorCnt++;
            if (ucErrorCnt == RFTU_MAX_RETRY)
            {
                printf("[SENDER Main] ERROR: Over the limit of sending times\n");
                close(nSocketFd);
                return RFTU_RET_ERROR;
            }
        }
        else
        {
            recvfrom(nSocketFd, &stRFTUPacketDataReceive, sizeof(stRFTUPacketDataReceive), 0, (struct sockaddr*)&stReceiverAddr, &socklen);
            /* Switch CMD field in the received message */
            switch(stRFTUPacketDataReceive.ucCmd)
            {
                case RFTU_CMD_READY:
                    printf("[SENDER Main] READY message received\n");
                    stPortInfoReceive = *((struct g_stPortInfo *) &stRFTUPacketDataReceive.ucData);
                    pth = (pthread_t *) malloc(stPortInfoReceive.ucNumberOfPort * sizeof(pthread_t));
                    stSenderParam = (struct g_stSenderParam *) malloc(stPortInfoReceive.ucNumberOfPort * sizeof(struct g_stSenderParam));
                    ulFSize = (unsigned long int *)malloc(stPortInfoReceive.ucNumberOfPort * sizeof(unsigned long int));
                    ulFPoint = (unsigned long int *)malloc(stPortInfoReceive.ucNumberOfPort * sizeof(unsigned long int));
                    MAIN_FileDiv(ulRFTUFileSize, ulFSize, ulFPoint, stPortInfoReceive.ucNumberOfPort);
                    for (i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                    {
                        printf("[SENDER Main] Size of portion %d of the file: %lu (bytes)\n", i+1, *(ulFSize + i));
                    }
                    usRFTUid = stRFTUPacketDataReceive.ucID;  /* Get transmission ID */

                    for (i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                    {
                        (stSenderParam + i)->nPortNumber = stPortInfoReceive.nPortNumber[i];
                        (stSenderParam + i)->unWindowSize = RFTU_WINDOW_SIZE;
                        (stSenderParam + i)->nFilePointerStart = *(ulFPoint + i);
                        (stSenderParam + i)->nFileSize = *(ulFSize + i);
                        (stSenderParam + i)->cThreadID = i;
                    }

                    /* Threads creation */
                    {
                        int *m;
                        int a = 0;
                        m = (int *) malloc(stPortInfoReceive.ucNumberOfPort * sizeof(int));
                        for (i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                        {
                            m[i] = pthread_create(pth + i, NULL, &SENDER_Start, (void *)(stSenderParam + i));
                            a = a | (*(m+i));
                        }
                        if (!a)
                        {
                            if (ucFlagVerbose)
                            {
                                printf("[SENDER Main] Thread Created.\n");
                            }
                            for (i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                            {
                                pthread_join(*(pth + i), NULL);
                            }
                            if (ucFlagVerbose)
                            {
                                printf("[SENDER Main] Thread functions are terminated.\n");
                            }
                        }
                        else
                        {
                            if (ucFlagVerbose)
                            {
                                printf("[SENDER Main] ERROR: Thread creation failed.\n");
                            }
                        }
                        free(m);
                    }
                    free(pth);
                    free(stSenderParam);
                    free(ulFSize);
                    free(ulFPoint);
                    close(nSocketFd);
                    return RFTU_RET_OK;
                case RFTU_CMD_NOSPACE:
                    printf("[SENDER Main] ERROR: No available space at receiver machine\n");
                    close(nSocketFd);
                    return RFTU_RET_ERROR;
                default:
                    break;
            }
        }
    }
}

char* SENDER_GetFileName(char *path)
{
    return basename(path);
}

unsigned long int SENDER_GetFileSize(char *path)
{
    unsigned long int sz;
    int fd;
    fd = open(path, O_RDONLY);
    sz = lseek(fd, (size_t)0, SEEK_END);
    close(fd);
    return sz;
}
