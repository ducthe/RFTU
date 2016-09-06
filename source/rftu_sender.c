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

    unsigned int  seq       = 0;
    unsigned char error_cnt = 0;
    unsigned char sending   = NO;

    fd_set fds; //set of file descriptors to be monitored by select()
    int select_result;
    struct timeval timeout;

    unsigned int Sb = 0; // sequence base
    unsigned int N = 0;  // size of window
    struct windows_t *windows = NULL;
    int i;


    // File info
    memcpy(file_info.filename, rftu_filename, sizeof(rftu_filename));
    file_info.filesize = rftu_filesize;

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
    FD_ZERO(&fds);  // Let the set becomes all zero
    FD_ADD(socket_fd, &fds);  // Add the socket_fd to the set

    /*---START---*/
    while(1)
    {
        if (sending == NO)
        {
            rftu_pkg_send.cmd = RFTU_CMD_INIT;
            rftu_pkg_send.data = (unsigned char *)file_info;
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
                //TODO: resend
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
                case (RFTU_CMD_READY):
                    if (sending == NO)
                    {
                        rftu_id = rftu_pkg_receive.id;
                        Sb = 0;
                        sending = YES;
                    }
                    break;
                case (RFTU_CMD_ACK):
                    if (sending == YES)
                    {
                        if (rftu_pkg_receive.seq > Sb)
                        {
                            error_cnt = 0;
                            Sb = rftu_pkg_receive.seq;
                            remove_package(windows, N, Sb);
                            //TODO: add and send
                        }
                    }
                    break;
                case (RFTU_CMD_NOSPACE):
                    printf("No available space at receiver machine.\n");
                    if (sending == NO)
                    {
                        break;
                    }
                    else
                    {
                        return RFTU_RET_ERROR;
                    }
                case (RFTU_CMD_COMPLETED):
                    if (sending == YES)
                    {
                        //TODO: closefile and close socket
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
