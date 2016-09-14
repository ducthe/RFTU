/**********************************************************************

 * Filename : rftu_receiver.c
 * Author: Peter + Richard
 * Contributor: Kevin
 * Date : 01-Sep-2016


 ***********************************************************************/
// Include
#include "rftu.h"

unsigned int BUFFER_SIZE = 20;


void* RECEIVER_Start(void* arg)
{
    // Variables
    struct sockaddr_in sender_soc, receiver_soc;
    int sd, fd; // socket descriptor and file descriptor

    struct rftu_packet_cmd_t  rftu_pkt_send_cmd;
    struct rftu_packet_data_t rftu_pkt_rcv;
    struct rftu_packet_data_t *rcv_buffer;
    unsigned int *currentsize_rcv_buffer = (unsigned int *) calloc(1, sizeof(unsigned int));

    struct timeval timeout; // set time out

    char receiving = YES;
    unsigned int Rn = 0;
    unsigned long int received_bytes = 0;

    fd_set set;
    int waiting;

    unsigned int number_ACK_loss = 0;
    

    // Local Variables
    unsigned char error_cnt = 0;
    socklen_t socklen = 0;

    struct g_stReceiverParam stReceiverParam = *(struct g_stReceiverParam *)arg;
    rcv_buffer = (struct rftu_packet_data_t*) calloc(BUFFER_SIZE, sizeof(struct rftu_packet_data_t));

    sd = socket(PF_INET, SOCK_DGRAM, 0); // socket DGRAM
    if(sd == -1)
    {
        printf("[RECEIVER] Error to create socket\n");
        return;
    }
    // init socket structure
    memset((char *) &receiver_soc, 0, sizeof(receiver_soc));
    receiver_soc.sin_family      = AF_INET;
    receiver_soc.sin_port        = htons(stReceiverParam.nPortNumber);
    receiver_soc.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(sender_soc);

    if (bind(sd, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0) // Create a socket for receiver
    {
        printf("[RECEIVER] Not binded\n");
        close(sd);
        return;
    }

    printf("%s\n\n", "[RECEIVER] Initializing receiver");

    /*---START---*/
    error_cnt = 0;
    receiving = YES;
    Rn = 0;
    received_bytes = 0;
    lseek(stReceiverParam.fd, stReceiverParam.nFilePointerStart, SEEK_SET); // seek file pointer to offset corresponding position
    while(1)
    {
        //Set time out
        timeout.tv_sec = RFTU_TIMEOUT;
        timeout.tv_usec = 0;

        // Wait Packet
        FD_ZERO (&set);
        FD_SET (sd, &set);
        waiting = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
        switch(waiting)
        {
            case 0: // time out
                if(receiving == YES)
                {
                    error_cnt++;
                    if (error_cnt == RFTU_MAX_RETRY)
                    {
                        printf("Error: Connection\n\n");

                        close(sd);
                        printf("[RECEIVER] Waiting for next files...\n");
                        receiving = NO;

                        error_cnt = 0;
                    }
                }
                break;
            case -1: // have an error
                printf("[RECEIVER] Error when waiting for package \n");
                break;
            default: // Read new packet
                recvfrom(sd, &rftu_pkt_rcv, sizeof(rftu_pkt_rcv), 0, (struct sockaddr *)&sender_soc, &socklen);
                // check the commanders
                switch(rftu_pkt_rcv.cmd)
                {
                    case (RFTU_CMD_DATA):
                        if (receiving == YES)
                        {
                            if (rftu_pkt_rcv.id == rftu_id)
                            {
                                // Resend ACK
                                if ((RECEIVER_isSeqExistInBuffer(rcv_buffer, BUFFER_SIZE, rftu_pkt_rcv.seq, currentsize_rcv_buffer) == YES)
                                        || (Rn > rftu_pkt_rcv.seq))
                                {
                                    if(flag_verbose == YES)
                                    {
                                        printf("%s%d\n", "[RECEIVER] Resend ACK seq: ", rftu_pkt_rcv.seq);
                                    }
                                    error_cnt = 0;
                                    rftu_pkt_send_cmd.cmd  = RFTU_CMD_ACK;
                                    rftu_pkt_send_cmd.seq  = rftu_pkt_rcv.seq;
                                    rftu_pkt_send_cmd.size = (received_bytes * 100 / stReceiverParam.nFileSize);
                                    sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 , (struct sockaddr *) &sender_soc, socklen);
                                }
                                else
                                {
                                    if(RECEIVER_IsFullBuffer(currentsize_rcv_buffer) == NO)
                                    {
                                        if(flag_verbose == YES)
                                        {
                                            printf("%s%d\n", "[RECEIVER] Received packet has sequence number = ", rftu_pkt_rcv.seq);
                                        }
                                        RECEIVER_InsertPacket(rcv_buffer, rftu_pkt_rcv, currentsize_rcv_buffer);
#ifdef DROPPER
                                        if (rand() % 20 == 0)
                                        {
                                            if(flag_verbose == YES)
                                            {
                                                printf("[RECEIVER] Dropped ACK seq: %u\n", rftu_pkt_rcv.seq);
                                            }
                                            number_ACK_loss++;
                                            continue;
                                        }
#endif
                                        // Send ACK
                                        if(flag_verbose == YES)
                                        {
                                            printf("%s%d\n", "[RECEIVER] Send ACK seq: ", rftu_pkt_rcv.seq);
                                        }
                                        error_cnt = 0;
                                        rftu_pkt_send_cmd.cmd  = RFTU_CMD_ACK;
                                        rftu_pkt_send_cmd.seq  = rftu_pkt_rcv.seq;
                                        rftu_pkt_send_cmd.size = (received_bytes * 100 / stReceiverParam.nFileSize);

                                        sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 , (struct sockaddr *) &sender_soc, socklen);
                                    }

                                    while(rcv_buffer[0].seq == Rn)
                                    {
                                        if(flag_verbose == YES)
                                        {
                                            printf("%s%d\n", "[RECEIVER] Writing packet had seq = ", rcv_buffer[0].seq);
                                        }
                                        write(stReceiverParam.fd, rcv_buffer[0].data, rcv_buffer[0].size);
                                        received_bytes += rcv_buffer[0].size;
                                        // printf("[RECEIVER] Got %lu bytes - %6.2f\n", received_bytes, (received_bytes * 100.0) / stReceiverParam.nFileSize);
                                        RECEIVER_ResetBuffer(rcv_buffer, currentsize_rcv_buffer);
                                        Rn++;
                                        error_cnt = 0;

                                        // When received file completly
                                        if (received_bytes == stReceiverParam.nFileSize)
                                        {
                                            printf("[RECEIVER] Sending COMPLETED cmd\n");

                                            // send complete command
                                            rftu_pkt_send_cmd.cmd  = RFTU_CMD_COMPLETED;
                                            rftu_pkt_send_cmd.id   = rftu_id;
                                            rftu_pkt_send_cmd.seq  = Rn;
                                            rftu_pkt_send_cmd.size = (received_bytes * 100 / stReceiverParam.nFileSize);

                                            sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 ,
                                                    (struct sockaddr *) &sender_soc, socklen);
                                            printf("[RECEIVER] File receiving completed\n");
                                            /* printf("[RECEIVER] Saved file to : %s\n\n", path); */
                                            printf("%s%d\n", "[RECEIVER] Total ACK loss: ", number_ACK_loss);
                                            // dont get any DATA segment
                                            receiving = NO;
                                            close(sd);
                                            return NULL;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                printf("[RECEIVER] Unknown ID: %i\n", rftu_pkt_rcv.id);
                            }
                        }
                        break;
                    default:
                        printf("Unknown command %u\n", rftu_pkt_rcv.cmd);
                        break;
                }
        }
    }
}

int RECEIVER_isSeqExistInBuffer(struct rftu_packet_data_t *rcv_buffer, unsigned int BUFFER_SIZE, unsigned int seq, unsigned int *currentsize_rcv_buffer)
{
    int i;
    for (i = 0; i < *currentsize_rcv_buffer; i++)
    {
        if(rcv_buffer[i].seq == seq)
        {
            return YES;
        }
    }
    return NO;
}


void RECEIVER_InsertPacket(struct rftu_packet_data_t *rcv_buffer, struct rftu_packet_data_t rftu_pkt_rcv, unsigned int *currentsize_rcv_buffer)
{
    int i, k;

    if(RECEIVER_IsEmptyBuffer(*currentsize_rcv_buffer) == YES)
    {
        rcv_buffer[0] = rftu_pkt_rcv;
        (*currentsize_rcv_buffer)++;
    }
    else
    {
        (*currentsize_rcv_buffer)++;
        for (i = 0; i < (*currentsize_rcv_buffer) - 1; i++)
        {
            if(rcv_buffer[i].seq > rftu_pkt_rcv.seq)
            {
                for(k = (*currentsize_rcv_buffer) - 1; k > i; k--)
                {
                    rcv_buffer[k] = rcv_buffer[k-1];
                }
                rcv_buffer[i] = rftu_pkt_rcv;
                return;
            }
        }
        // if its seq is max, add it in the end of current buffer
        rcv_buffer[i] = rftu_pkt_rcv;
    }
}

int RECEIVER_IsFullBuffer(unsigned int *currentsize_rcv_buffer)
{
    return (*currentsize_rcv_buffer == BUFFER_SIZE) ? YES : NO;
}

int RECEIVER_IsEmptyBuffer(unsigned int currentsize_rcv_buffer)
{
    return (currentsize_rcv_buffer == 0) ? YES : NO;
}

void RECEIVER_ResetBuffer(struct rftu_packet_data_t *rcv_buffer, unsigned int *currentsize_rcv_buffer)
{
    int i;
    (*currentsize_rcv_buffer)--;

    for (i = 0; i < *currentsize_rcv_buffer; i++)
    {
        rcv_buffer[i] = rcv_buffer[i + 1];
    }

}
