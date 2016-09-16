/*
 *   Filename    : rftu_receiver.c
 *   Author      : Richard, Peter (OCTO Team)
 *   Contributor : Kevin
 *   Date        : 06-Sep-2016
 */

#include "rftu.h"

unsigned int BUFFER_SIZE = 20;

void* RECEIVER_Start(void* arg)
{
    /* Variables */
    struct sockaddr_in sender_soc, receiver_soc;
    int nSocketFd;  /* Socket file descriptor */

    struct g_stRFTUPacketCmd  stRFTUPacketCmdSend;
    struct g_stRFTUPacketData stRFTUPacketDataRecv;
    struct g_stRFTUPacketData *stRFTUPacketDataBuffer;
    unsigned int *unRecvBufferSize = (unsigned int *) calloc(1, sizeof(unsigned int));

    struct timeval stTimeOut; /* Time out */

    char cReceiving = YES;
    unsigned int Rn = 0;
    unsigned long int ulReceivedBytes = 0;

    fd_set set;
    int nWaiting;

    unsigned int unAckLossNumber = 0;

    unsigned char ucErrorCnt = 0;
    socklen_t socklen = 0;

    struct g_stReceiverParam stReceiverParam = *(struct g_stReceiverParam *)arg;
    stRFTUPacketDataBuffer = (struct g_stRFTUPacketData*) calloc(BUFFER_SIZE, sizeof(struct g_stRFTUPacketData));

    nSocketFd = socket(PF_INET, SOCK_DGRAM, 0); /* DGRAM Socket */
    if(nSocketFd == -1)
    {
        printf("RECEIVER[%d] Error to create socket\n", stReceiverParam.cThreadID);
        return;
    }

    /* Init socket structure */
    memset((char *) &receiver_soc, 0, sizeof(receiver_soc));
    receiver_soc.sin_family      = AF_INET;
    receiver_soc.sin_port        = htons(stReceiverParam.nPortNumber);
    receiver_soc.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(sender_soc);

    if (bind(nSocketFd, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0)  /* Create a socket for receiver */
    {
        printf("RECEIVER[%d] Not binded\n", stReceiverParam.cThreadID);
        close(nSocketFd);
        return;
    }

    printf("RECEIVER[%d] Initializing receiver\n", stReceiverParam.cThreadID);

    /* ---START--- */
    ucErrorCnt = 0;
    cReceiving = YES;
    Rn = 0;
    ulReceivedBytes = 0;
    lseek(stReceiverParam.fd, stReceiverParam.nFilePointerStart, SEEK_SET); /* Seek file pointer to offset corresponding position */
    while(1)
    {
        /* Set time out */
        stTimeOut.tv_sec = RFTU_TIMEOUT;
        stTimeOut.tv_usec = 0;

        /* Wait for Packet */
        FD_ZERO (&set);
        FD_SET (nSocketFd, &set);
        nWaiting = select(FD_SETSIZE, &set, NULL, NULL, &stTimeOut);
        switch(nWaiting)
        {
            case 0: /* Time out */
                if(cReceiving == YES)
                {
                    ucErrorCnt++;
                    if (ucErrorCnt == RFTU_MAX_RETRY)
                    {
                        printf("Error: Connection\n\n");

                        close(nSocketFd);
                        printf("RECEIVER[%d] Waiting for next files...\n", stReceiverParam.cThreadID);
                        cReceiving = NO;

                        ucErrorCnt = 0;
                    }
                }
                break;
            case -1: /* Error */
                printf("RECEIVER[%d] ERROR: Error while waiting for DATA packets \n", stReceiverParam.cThreadID);
                printf("RECEIVER[%d] ERROR: %s\n\n", stReceiverParam.cThreadID, strerror(errno));
                break;
            default:  /* Read received packet */
                recvfrom(nSocketFd, &stRFTUPacketDataRecv, sizeof(stRFTUPacketDataRecv), 0, (struct sockaddr *)&sender_soc, &socklen);
                /* Check the commanders */
                switch(stRFTUPacketDataRecv.ucCmd)
                {
                    case (RFTU_CMD_DATA):
                        if (cReceiving == YES)
                        {
                            if (stRFTUPacketDataRecv.ucID == usRFTUid)
                            {
                                /* Resend ACK */
                                if ((RECEIVER_IsSeqExistInBuffer(stRFTUPacketDataBuffer, BUFFER_SIZE, stRFTUPacketDataRecv.unSeq, unRecvBufferSize) == YES)
                                        || (Rn > stRFTUPacketDataRecv.unSeq))
                                {
                                    if(ucFlagVerbose == YES)
                                    {
                                        printf("RECEIVER[%d] Resend ACK seq: %d\n", stReceiverParam.cThreadID, stRFTUPacketDataRecv.unSeq);
                                    }
                                    ucErrorCnt = 0;
                                    stRFTUPacketCmdSend.ucCmd  = RFTU_CMD_ACK;
                                    stRFTUPacketCmdSend.unSeq  = stRFTUPacketDataRecv.unSeq;
                                    stRFTUPacketCmdSend.usSize = (ulReceivedBytes * 100 / stReceiverParam.nFileSize);
                                    sendto(nSocketFd, &stRFTUPacketCmdSend, sizeof(stRFTUPacketCmdSend) , 0 , (struct sockaddr *) &sender_soc, socklen);
                                }
                                else
                                {
                                    if(RECEIVER_IsFullBuffer(unRecvBufferSize) == NO)
                                    {
                                        if(ucFlagVerbose == YES)
                                        {
                                            printf("RECEIVER[%d] Received packet has sequence number = %d\n", stReceiverParam.cThreadID, stRFTUPacketDataRecv.unSeq);
                                        }
                                        RECEIVER_InsertPacket(stRFTUPacketDataBuffer, stRFTUPacketDataRecv, unRecvBufferSize);
                                        /* DROP */
                                        if(ucFlagACKDrop == YES)
                                        {
                                            if (rand() % ((unsigned char)(100 / fAckLossRate)) == 0)
                                            {
                                                if(ucFlagVerbose == YES)
                                                {
                                                    printf("RECEIVER[%d] Dropped ACK seq: %u\n", stReceiverParam.cThreadID, stRFTUPacketDataRecv.unSeq);
                                                }
                                                unAckLossNumber++;
                                                continue;
                                            }
                                        }

                                        /* Send ACK */
                                        if(ucFlagVerbose == YES)
                                        {
                                            printf("RECEIVER[%d] Send ACK seq: %d\n", stReceiverParam.cThreadID, stRFTUPacketDataRecv.unSeq);
                                        }
                                        ucErrorCnt = 0;
                                        stRFTUPacketCmdSend.ucCmd  = RFTU_CMD_ACK;
                                        stRFTUPacketCmdSend.unSeq  = stRFTUPacketDataRecv.unSeq;
                                        stRFTUPacketCmdSend.usSize = (ulReceivedBytes * 100 / stReceiverParam.nFileSize);

                                        sendto(nSocketFd, &stRFTUPacketCmdSend, sizeof(stRFTUPacketCmdSend) , 0 , (struct sockaddr *) &sender_soc, socklen);
                                    }

                                    while(stRFTUPacketDataBuffer[0].unSeq == Rn)
                                    {
                                        if(ucFlagVerbose == YES)
                                        {
                                            printf("RECEIVER[%d] Writing packet had seq = %d\n", stReceiverParam.cThreadID, stRFTUPacketDataBuffer[0].unSeq);
                                        }
                                        write(stReceiverParam.fd, stRFTUPacketDataBuffer[0].ucData, stRFTUPacketDataBuffer[0].usSize);
                                        ulReceivedBytes += stRFTUPacketDataBuffer[0].usSize;
                                        // printf("RECEIVER[%d] Got %lu bytes - %6.2f\n", stReceiverParam.cThreadID, ulReceivedBytes, (ulReceivedBytes * 100.0) / stReceiverParam.nFileSize);
                                        RECEIVER_ResetBuffer(stRFTUPacketDataBuffer, unRecvBufferSize);
                                        Rn++;
                                        ucErrorCnt = 0;

                                        /* When the file is completely received */
                                        if (ulReceivedBytes == stReceiverParam.nFileSize)
                                        {
                                            printf("RECEIVER[%d] Sending COMPLETED cmd\n", stReceiverParam.cThreadID);

                                            /* Send complete command */
                                            stRFTUPacketCmdSend.ucCmd  = RFTU_CMD_COMPLETED;
                                            stRFTUPacketCmdSend.ucID   = usRFTUid;
                                            stRFTUPacketCmdSend.unSeq  = Rn;
                                            stRFTUPacketCmdSend.usSize = (ulReceivedBytes * 100 / stReceiverParam.nFileSize);

                                            sendto(nSocketFd, &stRFTUPacketCmdSend, sizeof(stRFTUPacketCmdSend) , 0 ,
                                                    (struct sockaddr *) &sender_soc, socklen);
                                            printf("RECEIVER[%d] File receiving completed\n", stReceiverParam.cThreadID);
                                            /* printf("RECEIVER[%d] Saved file to : %s\n\n", path); */
                                            printf("RECEIVER[%d] Total ACK loss: %d\n", stReceiverParam.cThreadID, unAckLossNumber);
                                            cReceiving = NO;
                                            close(nSocketFd);
                                            return NULL;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                printf("RECEIVER[%d] Unknown ID: %i\n", stReceiverParam.cThreadID, stRFTUPacketDataRecv.ucID);
                            }
                        }
                        break;
                    default:
                        printf("RECEIVER[%d] Unknown command %u\n", stReceiverParam.cThreadID, stRFTUPacketDataRecv.ucCmd);
                        break;
                }
        }
    }
}

int RECEIVER_IsSeqExistInBuffer(struct g_stRFTUPacketData *stRFTUPacketDataBuffer, unsigned int BUFFER_SIZE, unsigned int seq, unsigned int *unRecvBufferSize)
{
    int i;
    for (i = 0; i < *unRecvBufferSize; i++)
    {
        if(stRFTUPacketDataBuffer[i].unSeq == seq)
        {
            return YES;
        }
    }
    return NO;
}


void RECEIVER_InsertPacket(struct g_stRFTUPacketData *stRFTUPacketDataBuffer, struct g_stRFTUPacketData stRFTUPacketDataRecv, unsigned int *unRecvBufferSize)
{
    int i, k;

    if(RECEIVER_IsEmptyBuffer(*unRecvBufferSize) == YES)
    {
        stRFTUPacketDataBuffer[0] = stRFTUPacketDataRecv;
        (*unRecvBufferSize)++;
    }
    else
    {
        (*unRecvBufferSize)++;
        for (i = 0; i < (*unRecvBufferSize) - 1; i++)
        {
            if(stRFTUPacketDataBuffer[i].unSeq > stRFTUPacketDataRecv.unSeq)
            {
                for(k = (*unRecvBufferSize) - 1; k > i; k--)
                {
                    stRFTUPacketDataBuffer[k] = stRFTUPacketDataBuffer[k-1];
                }
                stRFTUPacketDataBuffer[i] = stRFTUPacketDataRecv;
                return;
            }
        }
        /* If its seq is max, add it in the end of current buffer */
        stRFTUPacketDataBuffer[i] = stRFTUPacketDataRecv;
    }
}

int RECEIVER_IsFullBuffer(unsigned int *unRecvBufferSize)
{
    return (*unRecvBufferSize == BUFFER_SIZE) ? YES : NO;
}

int RECEIVER_IsEmptyBuffer(unsigned int unRecvBufferSize)
{
    return (unRecvBufferSize == 0) ? YES : NO;
}

void RECEIVER_ResetBuffer(struct g_stRFTUPacketData *stRFTUPacketDataBuffer, unsigned int *unRecvBufferSize)
{
    int i;
    (*unRecvBufferSize)--;

    for (i = 0; i < *unRecvBufferSize; i++)
    {
        stRFTUPacketDataBuffer[i] = stRFTUPacketDataBuffer[i + 1];
    }

}
