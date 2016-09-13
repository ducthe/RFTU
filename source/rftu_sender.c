/*
 * Filename: rftu_sender.c
 * Author: OCTO team (Issac, Kevin)
 * Contributor: Richard
 * Date: 06-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

unsigned int number_pkt_loss = 0;

void* SENDER_Start(void *arg)
{
    struct rftu_packet_data_t rftu_pkg_send;    // package to be sent
    struct rftu_packet_data_t rftu_pkg_receive; // used to store received package

    int socket_fd; // socket file descriptor
    struct sockaddr_in receiver_addr; // receiver address

    unsigned int  Sn        = 0;    // sequence number
    unsigned char error_cnt = 0;
    unsigned char sending   = NO;
    int index_finded;

    fd_set fds; //set of file descriptors to be monitored by select()
    int select_result;
    struct timeval timeout;

    unsigned int Sb = 0; // sequence base
    unsigned int N = 0;  // size of window

    struct windows_t *windows = NULL;
    int file_fd;
    socklen_t socklen = 0;

    struct g_stSenderParam stSenderParam = *(struct g_stSenderParam *)arg;

    // Configure settings of the receiver address struct
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(stSenderParam.nPortNumber);
    if (inet_aton(rftu_ip, &receiver_addr.sin_addr) == 0)
    {
        printf("[SENDER] ERROR: The address is invalid\n");
        return;
    }
    memset(receiver_addr.sin_zero, '\0', sizeof(receiver_addr.sin_zero));
    socklen = sizeof(receiver_addr);

    // Socket creation
    if ((socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("[SENDER] ERROR: Socket creation fails\n");
        return;
    }

    // Specify value of window size N
    N = (rftu_filesize/RFTU_FRAME_SIZE) + 1;
    N = (N > stSenderParam.unWindowSize)? stSenderParam.unWindowSize : N;

    // Initialize sending window
    windows = (struct windows_t *) malloc(N * sizeof(struct windows_t));

    /*---START---*/
    error_cnt = 0;
    sending = NO;
    while(1)
    {
        if (sending == NO)
        {
            if((file_fd = open(rftu_filename, O_RDONLY)) == -1)
            {
                printf("[SENDER] ERROR: Openning file fails\n");
                free(windows);
                close(socket_fd);
                return;
            }
            Sb = -1;         // Set sequence base to -1
            Sn = 0;         // Set sequence number to 0
            sending = YES;  // let sending flag be YES

            // Sending the first window of data
            SENDER_AddAllPackages(windows, N, file_fd, &Sn);
            printf("[SENDER] Sending first window of data.\n");
            SENDER_Send_Packages(windows, N, socket_fd, &receiver_addr, NO);
        }
        // Initialize timeout
        timeout.tv_sec = RFTU_TIMEOUT;
        timeout.tv_usec = 0;

        // Set up the set of descriptors
        FD_ZERO(&fds);  // Let the file descriptor set becomes all zero
        FD_SET(socket_fd, &fds);  // Add the socket_fd to the set

        // Check time out using select() function
        select_result = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        if (select_result == -1) // Error
        {
            printf("[SENDER] ERROR: Error while waiting for packages\n");
            free(windows);
            close(socket_fd);
            return;
        }
        else if (select_result == 0) // Time out
        {
            error_cnt++;
            if (error_cnt == RFTU_MAX_RETRY)
            {
                printf("[SENDER] ERROR: Over the limit of sending times\n");
                free(windows);
                close(socket_fd);
                return;
            }
            if (sending == YES)
            {
                SENDER_Send_Packages(windows, N, socket_fd, &receiver_addr, YES);
            }
        }
        else // received characters from fds
        {
            recvfrom(socket_fd, &rftu_pkg_receive, sizeof(rftu_pkg_receive), 0, (struct sockaddr*)&receiver_addr, &socklen);

            // Switch the CMD fields in the received msg
            switch(rftu_pkg_receive.cmd)
            {
                case RFTU_CMD_ACK:
                    if (sending == YES)
                    {
                        if (flag_verbose)
                        {
                            printf("[SENDER] ACK sequence number received: %u\n", rftu_pkg_receive.seq);
                        }
                        // Set ACK flag for every packet received ACK
                        SENDER_SetACKflag(windows, N, rftu_pkg_receive.seq);
                        // Check seq = Sb + 1 in windows
                        while((index_finded = SENDER_FindPacketseq(windows, N, Sb + 1)) != RFTU_RET_ERROR)
                        {
                            Sb++;
                            SENDER_Add_Package(windows, N, file_fd, &Sn, index_finded);
                            SENDER_Send_Packages(windows, N, socket_fd, &receiver_addr, NO);
                            error_cnt = 0;
                        }
                    }
                    break;

                case RFTU_CMD_COMPLETED:
                    printf("[SENDER] File transfer completed.\n");
                    printf("%s%d\n", "[SENDER] Total packets loss: ", number_pkt_loss);
                    if (sending == YES)
                    {
                        free(windows);
                        close(file_fd);
                        close(socket_fd);
                        return NULL;
                    }
                    break;

                default:    // others
                    if (error_cnt == RFTU_MAX_RETRY)
                    {
                        printf("[SENDER] ERROR: Over the limit of sending times\n");
                        free(windows);
                        close(file_fd);
                        close(socket_fd);
                        return;
                    }
                    break;
            }
        }
    }
}


// When a packet was sent and sender received ack, sent flag = 1 and ack flag = 1
// Then this packet will be removed from windows, a new packet will be added to this position.
/**
 * [SENDER_SetACKflag description]
 * @param windows [description]
 * @param N       [description]
 * @param seq     [description]
 */

void SENDER_SetACKflag(struct windows_t *windows, unsigned char N, unsigned int seq)
{
    /* find packet with sequence number = received sequence number and mark it as ACK returned */
    int i;
    for(i = 0; i < N; i++)
    {
        if (windows[i].package.seq == seq)
        {
            windows[i].ack = YES;
            return;
        }
    }
}

int SENDER_FindPacketseq(struct windows_t *windows, unsigned char N, unsigned int seq)
{
    /* find packet with sequence number = received sequence number and mark it as ACK returned */
    int i;
    for(i = 0; i < N; i++)
    {
        if ((windows[i].package.seq == seq) && (windows[i].ack == YES))
        {
            return i;
        }
    }
    return RFTU_RET_ERROR;
}

void SENDER_AddAllPackages(struct windows_t *windows, unsigned char N, int file_fd, unsigned int *seq)
{
    int i;
    for(i = 0; i < N; i++)
    {
        /* Get size of packet using read() function
         * It returns the number of data bytes actually read,
         * which may be less than the number requested.*/
        int size_of_packet = 0;

        size_of_packet = read(file_fd, windows[i].package.data, RFTU_FRAME_SIZE);

        if(size_of_packet > 0)
        {
            windows[i].sent = NO;
            windows[i].ack  = NO;

            windows[i].package.cmd      = RFTU_CMD_DATA;
            windows[i].package.id       = rftu_id;
            windows[i].package.seq      = *seq;
            windows[i].package.size     = size_of_packet;

            (*seq)++;                   /* increase sequence number after add new packet*/
        }
    }
    printf("[SENDER] All packets are added into sending window.\n");
}

void SENDER_Add_Package(struct windows_t *windows, unsigned char N, int file_fd, unsigned int *seq, int index_finded)
{
    /* Get size of packet using read() function
     * It returns the number of data bytes actually read,
     * which may be less than the number requested.*/
    int size_of_packet = 0;

    size_of_packet = read(file_fd, windows[index_finded].package.data, RFTU_FRAME_SIZE);

    if(size_of_packet > 0)
    {
        windows[index_finded].sent = NO;
        windows[index_finded].ack  = NO;

        windows[index_finded].package.cmd      = RFTU_CMD_DATA;
        windows[index_finded].package.id       = rftu_id;
        windows[index_finded].package.seq      = *seq;
        windows[index_finded].package.size     = size_of_packet;

        (*seq)++;                   /* increase sequence number after add new packet*/
    }
}

/* SENDER_Send_Packages() function.
 * all = NO : only send the packets with windows[i].sent = NO
 * all = YES: send all packets in the windows regardless of the value of windows[i].sent
 */
void SENDER_Send_Packages(struct windows_t *windows, unsigned char N, int socket_fd, struct sockaddr_in *si_other, unsigned char all)
{
    int i;
    int pos_check = 0;   /* position check */

    for(i = 0; i < N; i++)
    {
        /* if send flag = NO: send the packet
         * if ack flag = NO && all = YES: resend the packet
         */
        if(windows[pos_check].sent == NO || (windows[pos_check].ack == NO && all == YES))
        {
            if(flag_verbose)
            {
                printf("[SENDER] Send DATA with sequence number: %u\n", windows[pos_check].package.seq);
            }
#ifdef DROPPER
            if(rand() % 20 == 0)
            {
                printf("[SENDER] Dropped packet with sequence number: %u\n", windows[pos_check].package.seq);
                windows[pos_check].sent = YES;
                number_pkt_loss++;
                continue;
            }
#endif
            sendto(socket_fd, &windows[pos_check].package, sizeof(struct rftu_packet_data_t), 0, (struct sockaddr *) si_other, (socklen_t) sizeof(*si_other));
            windows[pos_check].sent = YES;
        }
        pos_check++;
    }
}
