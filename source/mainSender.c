/*
 * Filename: mainSender.c
 * Author: OCTO team (Kevin)
 * Date: 13-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

unsigned char SENDER_Main(void)
{
    // const unsigned int RFTU_PORT[THREAD_NUMBER] = {8880, 8881, 8882, 8883, 8884, 8885, 8886, 8887};
    pthread_t pth[8];
    struct g_stSenderParam stSenderParam[8];

    // Sender variables
    struct g_stFileInfo stFileInfo;  // file info to be sent to receiver in INIT message
    struct g_stRFTUPacketData stRFTUPacketDataSend;    // package to be sent
    struct g_stRFTUPacketData stRFTUPacketDataReceive; // used to store received package
    struct g_stPortInfo stPortInfoReceive;     // used to store received information about port

    struct sockaddr_in ReceiverAddr; // receiver address
    socklen_t socklen = 0;
    int nSocketFd; // socket file descriptor

    unsigned char ucErrorCnt = 0;

    fd_set fds; //set of file descriptors to be monitored by select()
    int nSelectResult;
    struct timeval stTimeOut;

    char *temp;
    int i;

    unsigned long int *ulFSize;
    unsigned long int *ulFPoint;

    // File info setup
    temp = (char*)malloc(sizeof(cRFTUFileName));
    strcpy(temp, cRFTUFileName);
    strcpy(stFileInfo.cFileName, SENDER_Get_Filename(temp));
    free(temp);
    stFileInfo.ulFileSize = SENDER_Get_Filesize(cRFTUFileName);
    ulRFTUFileSize = stFileInfo.ulFileSize;

    printf("[SENDER Main] Filename is %s\n", stFileInfo.cFileName);
    printf("[SENDER Main] Filesize is %lu bytes\n", ulRFTUFileSize);

    /* ulFSize = (unsigned long int *)malloc(8 * sizeof(unsigned long int)); */
    /* ulFPoint = (unsigned long int *)malloc(8 * sizeof(unsigned long int)); */
    /* MAIN_div_file(ulRFTUFileSize, ulFSize, ulFPoint); */

    /* for (i = 0; i < THREAD_NUMBER; i++) */
    /* { */
    /*     printf("[SENDER Main] Size of portion %d of the file: %lu (bytes)\n", i+1, *(ulFSize + i)); */
    /* } */


    // Configure settings of the receiver address struct
    ReceiverAddr.sin_family = AF_INET;
    ReceiverAddr.sin_port = htons(RFTU_WELCOME_PORT); //8880
    if (inet_aton(cRFTUIP, &ReceiverAddr.sin_addr) == 0)
    {
        printf("[SENDER Main] ERROR: The address is invalid\n");
        return RFTU_RET_ERROR;
    }
    memset(ReceiverAddr.sin_zero, '\0', sizeof(ReceiverAddr.sin_zero));
    socklen = sizeof(ReceiverAddr);

    // Socket creation
    if ((nSocketFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("[SENDER Main] ERROR: Socket creation fails\n");
        return RFTU_RET_ERROR;
    }

    /*---START---*/
    ucErrorCnt = 0;
    while(1)
    {
        // Create INIT packet
        stRFTUPacketDataSend.ucCmd = RFTU_CMD_INIT;
        stRFTUPacketDataSend.ucID = 0;
        stRFTUPacketDataSend.unSeq = 0;
        stRFTUPacketDataSend.usSize = sizeof(stFileInfo);
        memcpy(stRFTUPacketDataSend.ucData, &stFileInfo, sizeof(stFileInfo));

        // Sent the INIT packet
        printf("[SENDER Main] Sending INIT message\n");
        sendto(nSocketFd, &stRFTUPacketDataSend, sizeof(stRFTUPacketDataSend), 0, (struct sockaddr*)&ReceiverAddr, socklen);

        // Initialize stTimeOut
        stTimeOut.tv_sec = RFTU_TIMEOUT;
        stTimeOut.tv_usec = 0;

        // Set up the set of descriptors
        FD_ZERO(&fds);  // Let the file descriptor set becomes all zero
        FD_SET(nSocketFd, &fds);  // Add the nSocketFd to the set

        // Check time out using select() function
        nSelectResult = select(FD_SETSIZE, &fds, NULL, NULL, &stTimeOut);
        if (nSelectResult == -1) // Error
        {
            printf("[SENDER Main] ERROR: Error while waiting for packets\n");
            printf("[SENDER Main] ERROR: %s", strerror(errno));
            close(nSocketFd);
            return RFTU_RET_ERROR;
        }
        else if (nSelectResult == 0) // Time out
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
            recvfrom(nSocketFd, &stRFTUPacketDataReceive, sizeof(stRFTUPacketDataReceive), 0, (struct sockaddr*)&ReceiverAddr, &socklen);
            // Switch the CMD fields in the received msg
            switch(stRFTUPacketDataReceive.ucCmd)
            {
                case RFTU_CMD_READY:
                    printf("[SENDER Main] READY message received\n");
                    stPortInfoReceive = *((struct g_stPortInfo *) &stRFTUPacketDataReceive.ucData);
                    ulFSize = (unsigned long int *)malloc(stPortInfoReceive.ucNumberOfPort * sizeof(unsigned long int));
                    ulFPoint = (unsigned long int *)malloc(stPortInfoReceive.ucNumberOfPort * sizeof(unsigned long int));
                    MAIN_div_file(ulRFTUFileSize, ulFSize, ulFPoint, stPortInfoReceive.ucNumberOfPort);
                    for (i = 0; i < THREAD_NUMBER; i++)
                    {
                        printf("[SENDER Main] Size of portion %d of the file: %lu (bytes)\n", i+1, *(ulFSize + i));
                    }
                    usRFTUid = stRFTUPacketDataReceive.ucID;  // Get transmission ID

                    for(i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                    {
                        stSenderParam[i].nPortNumber = stPortInfoReceive.nPortNumber[i];
                        stSenderParam[i].unWindowSize = RFTU_WINDOW_SIZE;
                        stSenderParam[i].nFilePointerStart = *(ulFPoint + i);
                        stSenderParam[i].nFileSize = *(ulFSize + i);
                        stSenderParam[i].cThreadID = i;
                    }

                    // Threads creation
                    {
                        int m[8];
                        for (i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                        {
                            m[i] = pthread_create(&pth[i], NULL, &SENDER_Start, (void *)&stSenderParam[i]);
                        }
                        if (!m[0] && !m[1] && !m[2] && !m[3] && !m[4] && !m[5] && !m[6] && !m[7])
                        {
                            if (ucFlagVerbose)
                            {
                                printf("[SENDER Main] Thread Created.\n");
                            }
                            for (i = 0; i < stPortInfoReceive.ucNumberOfPort; i++)
                            {
                                pthread_join(pth[i], NULL);
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
                    }
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

char* SENDER_Get_Filename(char *path)
{
    return basename(path);
}

unsigned long int SENDER_Get_Filesize(char *path)
{
    unsigned long int sz;
    int fd;
    fd = open(path, O_RDONLY);
    sz = lseek(fd, (size_t)0, SEEK_END);
    close(fd);
    return sz;
}
