/*
 * Filename: rftu_sender.c
 * Author: OCTO team (Issac, Kevin)
 * Date: 06-Sep-2016
 */
/*************************************************************************/
#include "../include/rftu.h"

unsigned char rftu_sender()
{
    // Sender variables
    struct fileInfo file_info;  // file info to be sent to receiver in INIT message
    struct rftu_package_data_t rftu_pkg_send;    // package to be sent
    struct rftu_package_data_t rftu_pkg_receive; // used to store received package

    int socket_fd; // socket file descriptor
    struct sockaddr_in receiver_addr; // receiver address

    unsigned int  Sn        = 0;    // sequence number
    unsigned char error_cnt = 0;
    unsigned char sending   = NO;

    fd_set fds; //set of file descriptors to be monitored by select()
    int select_result;
    struct timeval timeout;

    unsigned int Sb = 0; // sequence base
    unsigned int N = 0;  // size of window
    struct windows_t *windows = NULL;
    int i;
    int file_fd;


    // File info setup
    memcpy(file_info.filename, get_filename(rftu_filename), sizeof(get_filename(rftu_filename)));
    file_info.filesize = get_filesize(rftu_filename);
    rftu_filesize = file_info.filesize;

    // Socket creation
    if ((socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("Socket creation fails.\n");
        return RFTU_RET_ERROR;
    }

    // Configure settings of the receiver address struct
    receiver_addr.sin_family = AF_INET;
    receiver_addr.port = htons(RFTU_PORT);
    if (inet_aton(rftu_ip, &receiver_addr.sin_addr) == 0)
    {
        printf("The address is invalid.\n");
        return RFTU_RET_ERROR;
    }
    memset(receiver_addr.sin_zero, '\0', sizeof(receiver_addr.sin_zero));

    // Specify value of window size N
    N = (rftu_filesize/RFTU_SEGMENT_SIZE) + 1;
    N = (N > RFTU_WINDOW_SIZE)? RFTU_WINDOW_SIZE : N;

    // Initialize sending window
    windows = (struct windows_t*)malloc(sizeof(struct windows_t));
    for (i = 0; i < N; i++)
    {
        windows->sent = NO;
        windows->ack = NO;
    }

    // Initialize timeout
    timeout.tv_sec = RFTU_TIMEOUT;
    timeout.tv_usec = 0;

    // Set up the set of descriptors
    FD_ZERO(&fds);  // Let the file descriptor set becomes all zero
    FD_ADD(socket_fd, &fds);  // Add the socket_fd to the set

    /*---START---*/
    while(1)
    {
        if (sending == NO)
        {
            file_fd = open(rftu_filename, O_RDONLY);
            rftu_pkg_send.cmd = RFTU_CMD_INIT;
            rftu_pkg_send.data = (unsigned char *)&file_info;
            sendto(socket_fd, (const void*)&rftu_pkg_send, sizeof(rftu_pkg_send), 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
        }
        // Check time out using select() function
        select_result = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        if (select_result == -1) // Error
        {
            printf("Error while waiting for packages\n");
        }
        else if (select_result == 0) // Time out
        {
            error_cnt++;
            if (sending == YES)
            {
                send_packages(windows, N, socket_fd, &receiver_addr, YES);
            }
            if (error_cnt == RFTU_MAX_RETRY)
            {
                printf("Over maximum retry.\n");
                return RFTU_RET_ERROR;
            }
        }
        else // received characters from fds
        {
            recvfrom(socket_fd, (void*)&rftu_pkg_receive, sizeof(rftu_pkg_receive), 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
            switch(rftu_pkg_receive.cmd)
            {
                case RFTU_CMD_READY:
                    if (sending == NO)
                    {
                        rftu_id = rftu_pkg_receive.id;
                        Sb = 0;
                        Sn = 0;
                        sending = YES;
                    }
                    break;
                case RFTU_CMD_ACK:
                    if (sending == YES)
                    {
                        if (rftu_pkg_receive.seq > Sb)
                        {
                            error_cnt = 0;
                            Sb = rftu_pkg_receive.seq;
                            remove_package(windows, N, Sb);
                            add_packages(windows, N, file_fd, &Sn);
                            send_packages(windows, N, socket_fd, &receiver_addr, NO);
                        }
                    }
                    break;
                case RFTU_CMD_NOSPACE:
                    printf("No available space at receiver machine.\n");
                    if (sending == NO)
                    {
                        break;
                    }
                    else
                    {
                        return RFTU_RET_ERROR;
                    }
                case RFTU_CMD_COMPLETED:
                    if (sending == YES)
                    {
                        close(file_fd);
                        shutdown(socket_fd, 2);
                    }
                    break;
                default: // RFTU_CMD_ERROR
                    if (error_cnt == RFTU_MAX_RETRY)
                    {
                        printf("Over maximum retry.\n");
                        return RFTU_RET_ERROR;
                    }
                    break;
            }
        }
    }
}


char* get_filename(char *path)
{
    return basename(path);
}

unsigned long int get_filesize(char *path)
{
    unsigned long int sz;
    int fd;
    fd = open(path, O_RDONLY);
    sz = lseek(fd, (size_t)0, SEEK_END);
    close(fd);
    return sz;
}

// When a packet was sent and sender received ack, sent flag = 1 and ack flag = 1
// Then this packet will be removed from windows, a new packet will be added to this position.
void remove_package(struct windows_t *windows, unsigned char N, unsigned int seq)
{
    /* find all packets with sequence number lower than received sequence number and mark them all as ACK returned */
    int i;
    for(i = 0; i < N; i++)
    {
        if(windows[i].package.seq < seq)
            windows[i].ack = YES;
    }
}


void add_packages(struct windows_t *windows, unsigned char N, int file_fd, unsigned int *seq)
{
    int i;
    for(i = 0; i < N; i++)
    {
        if(windows[i].sent == YES && windows[i].ack == YES)
        {
            /* Get size of packet using read() function
             * It returns the number of data bytes actually read,
             * which may be less than the number requested.*/
            int size_of_packet = 0;

            size_of_packet= read(file_fd, windows[i].package.data, RFTU_SEGMENT_SIZE);

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
    }
}

/* send_packages() function.
 * send_packages(struct windows_t *windows, unsigned char N, int socket_fd, struct sockaddr_in *si_other, unsigned char all)
 * all = YES: send all packets in the windows regardless of the value of windows[i].sent
 * all = NO : only send the packets with windows[i].sent = NO
 */
void send_packages(struct windows_t *windows, unsigned char N, int socket_fd, struct sockaddr_in *si_other, unsigned char all)
{
    /* the packets error when without ack returned => resent : all = YES*/
    int i;
    static int pos_check = 0;   /* position check */

    for(i = 0; i < N; i++)
    {
        /* if send flag = NO: send the packets
         * if ack flag  = NO and all = YES (Resent required): Resend
         */
        if(windows[pos_check].sent == NO || (windows[pos_check].ack == NO && all = YES))
        {
            if(flag_verbose)
            {
                printf("[SENDER] Send DATA sequence number: %u\n", windows[pos_check].package.seq);
            }

            if(rand() % 4 == 0)
            {
                printf("[SENDER] Dropped packet sequence number: %u\n", windows[pos_check].package.seq);
                windows[pos_check].sent = YES;
            }
            else
            {
                sendto(socket_fd, &windows[pos_check].package, sizeof(struct rftu_package_data_t), 0, (struct sockaddr *) si_other, (socklen_t) sizeof(*si_other));
                windows[pos_check].sent = YES;
            }
        }
        pos_check++;
        /* pos_check = pos_check % 8;  #<{(| window size max = 8|)}># */
    }
}
