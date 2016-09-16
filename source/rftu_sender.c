/*
 *   Filename    : rftu_sender.c
 *   Author      : Kevin, Issac (OCTO Team)
 *   Contributor : Richard
 *   Date        : 06-Sep-2016
 */
#include "rftu.h"

unsigned int unNumberPktLoss = 0;

void* SENDER_Start(void *arg)
{
    struct g_stRFTUPacketData stRFTUPacketDataSend;  /* Packet to be sent */
    struct g_stRFTUPacketData stRFTUPacketDataRecv;  /* Used to store received package */

    int nSocketFd;                      /* Socket file descriptor */
    struct sockaddr_in ReceiverAddr;    /* Receiver address */

    unsigned int  Sn        = 0;        /* Sequence number */
    unsigned char ucErrorCnt = 0;
    unsigned char cSending   = NO;
    int nIndexFound;

    fd_set fds; /* Set of file descriptors to be monitored by select() */
    int nSelectResult;
    struct timeval stTimeOut;

    unsigned int Sb = 0; /* Sequence base */
    unsigned int N = 0;  /* Size of window */

    struct g_stWindows *stWindows = NULL;
    int file_fd;
    socklen_t socklen = 0;

    struct g_stSenderParam stSenderParam = *(struct g_stSenderParam *)arg;

    /* Configure settings of the receiver address struct */
    ReceiverAddr.sin_family = AF_INET;
    ReceiverAddr.sin_port = htons(stSenderParam.nPortNumber);
    if (inet_aton(cRFTUIP, &ReceiverAddr.sin_addr) == 0)
    {
        printf("SENDER[%d] ERROR: The address is invalid\n", stSenderParam.cThreadID);
        return;
    }
    memset(ReceiverAddr.sin_zero, '\0', sizeof(ReceiverAddr.sin_zero));
    socklen = sizeof(ReceiverAddr);

    /* Socket creation */
    if ((nSocketFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("SENDER[%d] ERROR: Socket creation fails\n", stSenderParam.cThreadID);
        return;
    }

    /* Specify value of window size N */
    N = (stSenderParam.nFileSize/RFTU_FRAME_SIZE) + 1;
    N = (N > stSenderParam.unWindowSize)? stSenderParam.unWindowSize : N;

    /* Initialize sending window */
    stWindows = (struct g_stWindows *) malloc(N * sizeof(struct g_stWindows));

    /* ---START--- */
    ucErrorCnt = 0;
    cSending = NO;
    while(1)
    {
        if (cSending == NO)
        {
            if((file_fd = open(cRFTUFileName, O_RDONLY)) == -1)
            {
                printf("SENDER[%d] ERROR: Openning file fails\n", stSenderParam.cThreadID);
                free(stWindows);
                close(nSocketFd);
                return;
            }

            /* Point to this position of thread */
            lseek(file_fd, (off_t)stSenderParam.nFilePointerStart, SEEK_SET);

            Sb = -1;
            Sn = 0;
            cSending = YES;

            // Sending the first window of data
            SENDER_AddAllPacket(stWindows, N, file_fd, &Sn);
            printf("SENDER[%d] All packets are added into sending window.\n", stSenderParam.cThreadID);
            printf("SENDER[%d] Sending first window of data.\n", stSenderParam.cThreadID);
            SENDER_SendPacket(stWindows, N, nSocketFd, &ReceiverAddr, NO, stSenderParam.cThreadID);
        }
        /* Initialize stTimeOut */
        stTimeOut.tv_sec = RFTU_TIMEOUT;
        stTimeOut.tv_usec = 0;

        /* Set up the set of descriptors */
        FD_ZERO(&fds);              /* Let the file descriptor set becomes all zero */
        FD_SET(nSocketFd, &fds);    /* Add the nSocketFd to the set */

        /* Check time out using select() function */
        nSelectResult = select(FD_SETSIZE, &fds, NULL, NULL, &stTimeOut);
        if (nSelectResult == -1) /* Error */
        {
            printf("SENDER[%d] ERROR: Error while waiting for packets\n", stSenderParam.cThreadID);
            printf("SENDER[%d] ERROR: %s\n\n", stSenderParam.cThreadID, strerror(errno));
            free(stWindows);
            close(nSocketFd);
            return;
        }
        else if (nSelectResult == 0) /* Time out */
        {
            ucErrorCnt++;
            if (ucErrorCnt == RFTU_MAX_RETRY)
            {
                printf("SENDER[%d] ERROR: Over the limit of sending times\n", stSenderParam.cThreadID);
                free(stWindows);
                close(nSocketFd);
                return;
            }
            if (cSending == YES)
            {
                SENDER_SendPacket(stWindows, N, nSocketFd, &ReceiverAddr, YES, stSenderParam.cThreadID);
            }
        }
        else /* Received characters from fds */
        {
            recvfrom(nSocketFd, &stRFTUPacketDataRecv, sizeof(stRFTUPacketDataRecv), 0, (struct sockaddr*)&ReceiverAddr, &socklen);

            /* Switch the CMD fields in the received msg */
            switch(stRFTUPacketDataRecv.ucCmd)
            {
                case RFTU_CMD_ACK:
                    if (cSending == YES)
                    {
                        if (ucFlagVerbose)
                        {
                            printf("SENDER[%d] ACK sequence number received: %u\n", stSenderParam.cThreadID, stRFTUPacketDataRecv.unSeq);
                        }
                        /* Set ACK flag for every packet received ACK */
                        SENDER_SetAckFlag(stWindows, N, stRFTUPacketDataRecv.unSeq);
                        /* Check seq = Sb + 1 in stWindows */
                        while((nIndexFound = SENDER_FindPacketSeq(stWindows, N, Sb + 1)) != RFTU_RET_ERROR)
                        {
                            Sb++;
                            SENDER_AddPacket(stWindows, N, file_fd, &Sn, nIndexFound);
                            SENDER_SendPacket(stWindows, N, nSocketFd, &ReceiverAddr, NO, stSenderParam.cThreadID);
                            ucErrorCnt = 0;
                        }
                    }
                    break;

                case RFTU_CMD_COMPLETED:
                    printf("SENDER[%d] File transfer completed.\n", stSenderParam.cThreadID);
                    printf("SENDER[%d] Total packets loss: %d\n", stSenderParam.cThreadID, unNumberPktLoss);
                    if (cSending == YES)
                    {
                        free(stWindows);
                        close(file_fd);
                        close(nSocketFd);
                        return NULL;
                    }
                    break;

                default:   /* Others */
                    if (ucErrorCnt == RFTU_MAX_RETRY)
                    {
                        printf("SENDER[%d] ERROR: Over the limit of sending times\n", stSenderParam.cThreadID);
                        free(stWindows);
                        close(file_fd);
                        close(nSocketFd);
                        return;
                    }
                    break;
            }
        }
    }
}

/*
 *   When a packet was sent and sender received ack, sent flag = 1 and ack flag = 1
 *   Then this packet will be removed from windows, a new packet will be added to this position.
 */
void SENDER_SetAckFlag(struct g_stWindows *stWindows, unsigned char N, unsigned int seq)
{
    /* Find packet with sequence number = received sequence number and mark it as ACK returned */
    int i;
    for(i = 0; i < N; i++)
    {
        if (stWindows[i].stRFTUPacketData.unSeq == seq)
        {
            stWindows[i].ucAck = YES;
            return;
        }
    }
}

int SENDER_FindPacketSeq(struct g_stWindows *stWindows, unsigned char N, unsigned int seq)
{
    /* Find packet with sequence number = received sequence number and mark it as ACK returned */
    int i;
    for(i = 0; i < N; i++)
    {
        if ((stWindows[i].stRFTUPacketData.unSeq == seq) && (stWindows[i].ucAck == YES))
        {
            return i;
        }
    }
    return RFTU_RET_ERROR;
}

void SENDER_AddAllPacket(struct g_stWindows *stWindows, unsigned char N, int file_fd, unsigned int *seq)
{
    int i;
    for(i = 0; i < N; i++)
    {
        /*
         *   Get size of packet using read() function
         *   It returns the number of data bytes actually read,
         *   which may be less than the number requested.
         */
        int nPacketSize = 0;

        nPacketSize = read(file_fd, stWindows[i].stRFTUPacketData.ucData, RFTU_FRAME_SIZE);

        if(nPacketSize > 0)
        {
            stWindows[i].ucSent = NO;
            stWindows[i].ucAck  = NO;

            stWindows[i].stRFTUPacketData.ucCmd      = RFTU_CMD_DATA;
            stWindows[i].stRFTUPacketData.ucID       = usRFTUid;
            stWindows[i].stRFTUPacketData.unSeq      = *seq;
            stWindows[i].stRFTUPacketData.usSize     = nPacketSize;

            (*seq)++;   /* Increase sequence number after add new packet */
        }
    }
}

void SENDER_AddPacket(struct g_stWindows *stWindows, unsigned char N, int file_fd, unsigned int *seq, int nIndexFound)
{
    /*
     *   Get size of packet using read() function
     *   It returns the number of data bytes actually read,
     *   which may be less than the number requested.
     */
    int nPacketSize = 0;

    nPacketSize = read(file_fd, stWindows[nIndexFound].stRFTUPacketData.ucData, RFTU_FRAME_SIZE);

    if(nPacketSize > 0)
    {
        stWindows[nIndexFound].ucSent = NO;
        stWindows[nIndexFound].ucAck  = NO;

        stWindows[nIndexFound].stRFTUPacketData.ucCmd      = RFTU_CMD_DATA;
        stWindows[nIndexFound].stRFTUPacketData.ucID       = usRFTUid;
        stWindows[nIndexFound].stRFTUPacketData.unSeq      = *seq;
        stWindows[nIndexFound].stRFTUPacketData.usSize     = nPacketSize;

        (*seq)++;   /* Increase sequence number after add new packet */
    }
}

/*
*   SENDER_SendPacket() function.
*   all = NO : only send the packets with stWindows[i].sent = NO
*   all = YES: send all packets in the stWindows regardless of the value of stWindows[i].sent
*/
void SENDER_SendPacket(struct g_stWindows *stWindows, unsigned char N, int nSocketFd, struct sockaddr_in *si_other, unsigned char all, char cThreadID)
{
    int i;
    int pos_check = 0;   /* Position check */

    for(i = 0; i < N; i++)
    {
        /*
        *   If send flag = NO: send the packet
        *   If ack flag = NO && all = YES: resend the packet
        */
        if(stWindows[pos_check].ucSent == NO || (stWindows[pos_check].ucAck == NO && all == YES))
        {
            if(ucFlagVerbose)
            {
                printf("SENDER[%d] Send DATA with sequence number: %u\n", cThreadID, stWindows[pos_check].stRFTUPacketData.unSeq);
            }
            /* DROP */
            if(ucFlagPacketDrop == YES)
            {
                if(rand() % ((unsigned char)(100 / fPacketLossRate)) == 0)
                {
                    printf("SENDER[%d] Dropped packet with sequence number: %u\n", cThreadID, stWindows[pos_check].stRFTUPacketData.unSeq);
                    stWindows[pos_check].ucSent = YES;
                    unNumberPktLoss++;
                    continue;
                }
            }
            sendto(nSocketFd, &stWindows[pos_check].stRFTUPacketData, sizeof(struct g_stRFTUPacketData), 0, (struct sockaddr *) si_other, (socklen_t) sizeof(*si_other));
            stWindows[pos_check].ucSent = YES;
        }
        pos_check++;
    }
}
