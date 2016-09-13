/**********************************************************************

 * Filename : rftu_receiver.c
 * Author: Peter + Richard
 * Contributor: Kevin
 * Date : 01-Sep-2016


 ***********************************************************************/
// Include
#include "rftu.h"


// Variables

char path[255] = "/";

struct sockaddr_in sender_soc, receiver_soc;
int sd, fd; // socket descriptor and file descriptor


struct file_info_t file_info; // File transfered
struct rftu_packet_cmd_t  rftu_pkt_send_cmd;
struct rftu_packet_data_t rftu_pkt_rcv;
struct rftu_packet_data_t *rcv_buffer;
unsigned int currentsize_rcv_buffer;

struct timeval timeout; // set time out
char receiving = NO;
fd_set set;
int waiting;
unsigned char error_cnt = 0;
unsigned int Rn;
unsigned int number_ACK_loss = 0;
unsigned short rftu_id;
unsigned long int received_bytes;
socklen_t socklen;
unsigned long int   rftu_filesize;
unsigned int BUFFER_SIZE = 20;

unsigned char RECEIVER_Start(void)
{
    currentsize_rcv_buffer = 0;
    //new line
    rcv_buffer = (struct rftu_packet_data_t*) calloc(BUFFER_SIZE, sizeof(struct rftu_packet_data_t));

    printf("%s\n\n", "[RECEIVER] Initialize receiver");
    sd = socket(AF_INET, SOCK_DGRAM, 0); // socket DGRAM
    if(sd == -1)
    {
        printf("[RECEIVER] Error to create socket\n");
        return RFTU_RET_ERROR;
    }
    // init socket structure
    memset((char *) &receiver_soc, 0, sizeof(receiver_soc));
    receiver_soc.sin_family      = AF_INET;
    receiver_soc.sin_port        = htons(RFTU_PORT);
    receiver_soc.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(sender_soc);

    if (bind(sd, (struct sockaddr *)&receiver_soc, sizeof(receiver_soc)) != 0) // Create a socket for receiver
    {
        printf("[RECEIVER] Not binded\n");
        return RFTU_RET_ERROR;
    }

    while(1)
    {
        //Set time outcd
        timeout.tv_sec = RFTU_TIMEOUT;
        timeout.tv_usec = 0;
        // Wait Packet
        FD_ZERO (&set);
        FD_SET (sd, &set);
        waiting = select(FD_SETSIZE, &set, NULL, NULL,
                &timeout);
        switch(waiting)
        {
            case 0: // time out
                if(receiving == YES)
                {
                    error_cnt++;
                }
                break;
            case -1: // have an error
                printf("[RECEIVER] Error when waiting for package \n");
                break;
            default: // Read new packet
                recvfrom(sd, &rftu_pkt_rcv, sizeof(rftu_pkt_rcv), 0, \
                        (struct sockaddr *)&sender_soc, &socklen);
                // check the commanders
                switch(rftu_pkt_rcv.cmd)
                {
                    case (RFTU_CMD_INIT):
                        if(receiving == NO)
                        {
                            // Open file
                            file_info = *((struct file_info_t *) &rftu_pkt_rcv.data);
                            printf("File info:\n File name : %s, Filesize: %ld bytes \n", file_info.filename, file_info.filesize);
                            // Create the file to save
                            strcpy(path,"/home/");
                            strcat(path, getlogin());
                            strcat(path, "/Desktop/");
                            strcat(path, file_info.filename);
                            fd = open(path,  O_CREAT | O_WRONLY,  0666);
                            printf("[RECEIVER] Saving file to : %s\n",path );
                            if (fd < 0)
                            {
                                printf("[RECEIVER] There is nospace, cannot create the file\n");
                                // Send command NOSPACE to sender
                                rftu_pkt_send_cmd.cmd = RFTU_CMD_NOSPACE;
                                sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 ,
                                        (struct sockaddr *) &sender_soc, socklen);
                            }
                            else
                            {
                                // Send command READY to sender
                                rftu_pkt_send_cmd.cmd = RFTU_CMD_READY;
                                rftu_pkt_send_cmd.id = rand();
                                rftu_id = rftu_pkt_send_cmd.id;
                                printf("READY cmd.id: %d\n", rftu_id );
                                sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 ,
                                        (struct sockaddr *) &sender_soc, socklen);

                                receiving = YES;
                                received_bytes = 0;
                                Rn = 0;

                            }
                        }
                        break;
                    case (RFTU_CMD_DATA):
                        if (receiving == YES)
                        {
                            if (rftu_pkt_rcv.id == rftu_id)
                            {
                                //resend ACK
                                // int RECEIVER_isSeqExistInBuffer(struct rftu_packet_data_t *rcv_buffer, int BUFFER_SIZE, unsigned int seq)
                                if ((RECEIVER_isSeqExistInBuffer(rcv_buffer, BUFFER_SIZE, rftu_pkt_rcv.seq) == YES)
                                        || (Rn > rftu_pkt_rcv.seq))
                                {
                                    printf("%s%d\n", "[RECEIVER] Resend ACK seq: ", rftu_pkt_rcv.seq);
                                    error_cnt = 0;
                                    rftu_pkt_send_cmd.cmd  = RFTU_CMD_ACK;
                                    rftu_pkt_send_cmd.seq  = rftu_pkt_rcv.seq;
                                    rftu_pkt_send_cmd.size = (received_bytes * 100 / file_info.filesize);
                                }
                                else
                                {
                                    if(RECEIVER_IsFullBuffer() == NO)
                                    {
                                        printf("%s%d\n", "[RECEIVER] Received packet had seq = ", rcv_buffer[0].seq);
                                        RECEIVER_InsertPacket(rcv_buffer, rftu_pkt_rcv);
                                        // #ifdef DROPPER
                                        //                                      if (rand() % 20 == 0)
                                        //                                      {
                                        //                                          printf("[RECEIVER] Dropped ACK seq: %u\n", rftu_pkt_rcv.seq);
                                        //                                          number_ACK_loss++;
                                        //                                          continue;
                                        //                                      }
                                        // #endif
                                        // return ACK
                                        printf("%s%d\n", "[RECEIVER] Send ACK seq: ", rftu_pkt_rcv.seq);
                                        error_cnt = 0;
                                        rftu_pkt_send_cmd.cmd  = RFTU_CMD_ACK;
                                        rftu_pkt_send_cmd.seq  = rftu_pkt_rcv.seq;
                                        rftu_pkt_send_cmd.size = (received_bytes * 100 / file_info.filesize);

                                        sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 ,
                                                (struct sockaddr *) &sender_soc, socklen);
                                    }
                                    while(rcv_buffer[0].seq == Rn)
                                    {
                                        printf("%s%d\n", "[RECEIVER] Writing packet had seq = ", rcv_buffer[0].seq);
                                        write(fd, rcv_buffer[0].data, rcv_buffer[0].size);
                                        received_bytes += rcv_buffer[0].size;
                                        // printf("[RECEIVER] Got %lu bytes - %6.2f\n", received_bytes, (received_bytes * 100.0) / rftu_filesize);
                                        RECEIVER_ResetBuffer(rcv_buffer);
                                        Rn++;

                                        // When received file completly
                                        if (received_bytes == file_info.filesize)
                                        {

                                            printf("[RECEIVER] Sending COMPLETED cmd\n");

                                            // send complete command
                                            rftu_pkt_send_cmd.cmd  = RFTU_CMD_COMPLETED;
                                            rftu_pkt_send_cmd.id   = rftu_id;
                                            rftu_pkt_send_cmd.seq  = Rn;
                                            rftu_pkt_send_cmd.size = (received_bytes * 100 / file_info.filesize);

                                            sendto(sd, &rftu_pkt_send_cmd, sizeof(rftu_pkt_send_cmd) , 0 ,
                                                    (struct sockaddr *) &sender_soc, socklen);

                                            // close file
                                            close(fd);
                                            printf("[RECEIVER] File receiving completed\n");
                                            printf("[RECEIVER] Saved file to : %s\n\n",path );
                                            printf("%s%d\n", "[RECEIVER] Total ACK loss: ", number_ACK_loss);
                                            printf("[RECEIVER] Waiting for next files...\n");

                                            // dont get any DATA segment
                                            receiving = NO;
                                        }

                                        error_cnt = 0;
                                    }
                                    // }
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

    if (receiving == YES)
    {
        if (error_cnt == RFTU_MAX_RETRY)
        {
            printf("Error: Connection\n\n");

            // close file
            close(fd);
            printf(" Waiting... for INIT command\n");
            receiving = NO;

            error_cnt = 0;
        }
    }
}


close(sd);
return RFTU_RET_OK;
}

int RECEIVER_isSeqExistInBuffer(struct rftu_packet_data_t *rcv_buffer, unsigned int BUFFER_SIZE, unsigned int seq)
{
    int i;
    for (i = 0; i < currentsize_rcv_buffer; i++)
    {
        if(rcv_buffer[i].seq == seq)
        {
            return YES;
        }
    }
    return NO;
}


void RECEIVER_InsertPacket(struct rftu_packet_data_t *rcv_buffer, struct rftu_packet_data_t rftu_pkt_rcv)
{
    int i, k;

    if(RECEIVER_IsEmptyBuffer() == YES)
    {
        rcv_buffer[0] = rftu_pkt_rcv;
    }
    else
    {
        for (i = 0; i < currentsize_rcv_buffer; i++)
        {
            if(rcv_buffer[i].seq > rftu_pkt_rcv.seq)
            {
                for(k = currentsize_rcv_buffer; k > i; k--)
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
    currentsize_rcv_buffer++;
}

int RECEIVER_IsFullBuffer()
{
    return (currentsize_rcv_buffer == BUFFER_SIZE) ? YES : NO;
}

int RECEIVER_IsEmptyBuffer()
{
    return (currentsize_rcv_buffer == 0) ? YES : NO;
}

void RECEIVER_ResetBuffer(struct rftu_packet_data_t *rcv_buffer)
{
    int i;
    currentsize_rcv_buffer--;

    for (i = 0; i < currentsize_rcv_buffer; i++)
    {
        rcv_buffer[i] = rcv_buffer[i + 1];
    }

}
