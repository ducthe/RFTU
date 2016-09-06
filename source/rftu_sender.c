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
    socklen_t slen = sizeof(receiver_addr);

    unsigned int  seq            = 0;
    unsigned char error_counter  = 0;
    unsigned char flag_sending   = NO;

    fd_set fds; //set of file descriptors to be monitored by select()
    int select_result;
    struct timeval timeout;

    unsigned int Sb = 0; // sequence base
    unsigned int N = 0;  // size of window
    struct windows_t *windows = NULL;
    int i;


    // File Info
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
    if (inet_aton(&rftu_ip, &receiver_addr.sin_addr) == 0)
    {
        printf("...\n");
        return RFTU_RET_ERROR;
    }
    memset(receiver_addr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

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
}
