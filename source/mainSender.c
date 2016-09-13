/*
 * Filename: mainSender.c
 * Author: OCTO team (Kevin)
 * Date: 13-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

unsigned char SENDER_Main(void)
{
    pthread_t pth;
    struct g_stSenderParam stSenderParam;
    stSenderParam.nPortNumber = RFTU_PORT; //8888
    stSenderParam.unWindowSize = RFTU_WINDOW_SIZE;

    // Sender variables
    struct file_info_t file_info;  // file info to be sent to receiver in INIT message
    struct rftu_packet_data_t rftu_pkg_send;    // package to be sent
    struct rftu_packet_data_t rftu_pkg_receive; // used to store received package

    struct sockaddr_in receiver_addr; // receiver address
    socklen_t socklen = 0;
    int socket_fd; // socket file descriptor

    unsigned char error_cnt = 0;

    fd_set fds; //set of file descriptors to be monitored by select()
    int select_result;
    struct timeval timeout;

    char *temp;

    // File info setup
    temp = (char*)malloc(sizeof(rftu_filename));
    strcpy(temp, rftu_filename);
    strcpy(file_info.filename, SENDER_Get_Filename(temp));
    free(temp);
    file_info.filesize = SENDER_Get_Filesize(rftu_filename);
    rftu_filesize = file_info.filesize;

    printf("[SENDER] Filename is %s\n", file_info.filename);
    printf("[SENDER] Filesize is %lu bytes\n", rftu_filesize);

    // Configure settings of the receiver address struct
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(RFTU_WELCOME_PORT); //8880
    if (inet_aton(rftu_ip, &receiver_addr.sin_addr) == 0)
    {
        printf("[SENDER] ERROR: The address is invalid\n");
        return RFTU_RET_ERROR;
    }
    memset(receiver_addr.sin_zero, '\0', sizeof(receiver_addr.sin_zero));
    socklen = sizeof(receiver_addr);

    // Socket creation
    if ((socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("[SENDER] ERROR: Socket creation fails\n");
        return RFTU_RET_ERROR;
    }

    /*---START---*/
    error_cnt = 0;
    while(1)
    {
        // Create INIT packet
        rftu_pkg_send.cmd = RFTU_CMD_INIT;
        rftu_pkg_send.id = 0;
        rftu_pkg_send.seq = 0;
        rftu_pkg_send.size = sizeof(file_info);
        memcpy(rftu_pkg_send.data, &file_info, sizeof(file_info));

        // Sent the INIT packet
        printf("[SENDER] Sending INIT message\n");
        sendto(socket_fd, &rftu_pkg_send, sizeof(rftu_pkg_send), 0, (struct sockaddr*)&receiver_addr, socklen);

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
            close(socket_fd);
            return RFTU_RET_ERROR;
        }
        else if (select_result == 0) // Time out
        {
            error_cnt++;
            if (error_cnt == RFTU_MAX_RETRY)
            {
                printf("[SENDER] ERROR: Over the limit of sending times\n");
                close(socket_fd);
                return RFTU_RET_ERROR;
            }
        }
        else
        {
            recvfrom(socket_fd, &rftu_pkg_receive, sizeof(rftu_pkg_receive), 0, (struct sockaddr*)&receiver_addr, &socklen);
            // Switch the CMD fields in the received msg
            switch(rftu_pkg_receive.cmd)
            {
                case RFTU_CMD_READY:
                    printf("[SENDER] READY message received\n");
                    rftu_id = rftu_pkg_receive.id;  // Get transmission ID
                    // Threads creation
                    {
                        int m;
                        m = pthread_create(&pth, NULL, &SENDER_Start, (void *)&stSenderParam);
                        if (!m)
                        {
                            printf("[SENDER] Thread created.\n");
                            pthread_join(pth, NULL);
                        }
                        else
                            printf("[SENDER] ERROR: Thread creation failed.\n");
                    }
                    close(socket_fd);
                    return RFTU_RET_OK;
                case RFTU_CMD_NOSPACE:
                    printf("[SENDER] ERROR: No available space at receiver machine\n");
                    close(socket_fd);
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
