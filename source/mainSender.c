/*
 * Filename: mainSender.c
 * Author: OCTO team (Kevin)
 * Date: 13-Sep-2016
 */
/*************************************************************************/
#include "rftu.h"

unsigned char SENDER_Main(void)
{
    pthread_t pth[8];
    struct g_stSenderParam stSenderParam[8];

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
    int thread_index;

    unsigned long int *fsize;
    unsigned long int *fpoint;

    // File info setup
    temp = (char*)malloc(sizeof(rftu_filename));
    strcpy(temp, rftu_filename);
    strcpy(file_info.filename, SENDER_Get_Filename(temp));
    free(temp);
    file_info.filesize = SENDER_Get_Filesize(rftu_filename);
    rftu_filesize = file_info.filesize;

    printf("[SENDER Main] Filename is %s\n", file_info.filename);
    printf("[SENDER Main] Filesize is %lu bytes\n", rftu_filesize);

    // Divide original file to 8 part
    fsize = (unsigned long int *)malloc(8 * sizeof(unsigned long int));
    fpoint = (unsigned long int *)malloc(8 * sizeof(unsigned long int));
    MAIN_div_file(rftu_filesize, fsize, fpoint);

    printf("[SENDER Main] Size of portion 1 of the file: %lu (bytes)\n", *(fsize + 0));
    printf("[SENDER Main] Size of portion 2 of the file: %lu (bytes)\n", *(fsize + 1));
    printf("[SENDER Main] Size of portion 3 of the file: %lu (bytes)\n", *(fsize + 2));
    printf("[SENDER Main] Size of portion 4 of the file: %lu (bytes)\n", *(fsize + 3));
    printf("[SENDER Main] Size of portion 5 of the file: %lu (bytes)\n", *(fsize + 4));
    printf("[SENDER Main] Size of portion 6 of the file: %lu (bytes)\n", *(fsize + 5));
    printf("[SENDER Main] Size of portion 7 of the file: %lu (bytes)\n", *(fsize + 6));
    printf("[SENDER Main] Size of portion 8 of the file: %lu (bytes)\n", *(fsize + 7));

    // Param of thread sender [0]
    stSenderParam[0].nPortNumber = RFTU_PORT_0;
    stSenderParam[0].unWindowSize = 8*RFTU_WINDOW_SIZE;
    stSenderParam[0].nFilePointerStart = *(fpoint + 0);
    stSenderParam[0].nFileSize = *(fsize + 0);
    stSenderParam[0].cThreadID = 0;

    // Param of thread sender [1]
    stSenderParam[1].nPortNumber = RFTU_PORT_1;
    stSenderParam[1].unWindowSize = 6*RFTU_WINDOW_SIZE;
    stSenderParam[1].nFilePointerStart = *(fpoint + 1);
    stSenderParam[1].nFileSize = *(fsize + 1);
    stSenderParam[1].cThreadID = 1;

    // Param of thread sender [2]
    stSenderParam[2].nPortNumber = RFTU_PORT_2;
    stSenderParam[2].unWindowSize = 4*RFTU_WINDOW_SIZE;
    stSenderParam[2].nFilePointerStart = *(fpoint + 2);
    stSenderParam[2].nFileSize = *(fsize + 2);
    stSenderParam[2].cThreadID = 2;

    // Param of thread sender [3]
    stSenderParam[3].nPortNumber = RFTU_PORT_3;
    stSenderParam[3].unWindowSize = 4*RFTU_WINDOW_SIZE;
    stSenderParam[3].nFilePointerStart = *(fpoint + 3);
    stSenderParam[3].nFileSize = *(fsize + 3);
    stSenderParam[3].cThreadID = 3;

    // Param of thread sender [4]
    stSenderParam[4].nPortNumber = RFTU_PORT_4;
    stSenderParam[4].unWindowSize = 2*RFTU_WINDOW_SIZE;
    stSenderParam[4].nFilePointerStart = *(fpoint + 4);
    stSenderParam[4].nFileSize = *(fsize + 4);
    stSenderParam[4].cThreadID = 4;

    // Param of thread sender [5]
    stSenderParam[5].nPortNumber = RFTU_PORT_5;
    stSenderParam[5].unWindowSize = 2*RFTU_WINDOW_SIZE;
    stSenderParam[5].nFilePointerStart = *(fpoint + 5);
    stSenderParam[5].nFileSize = *(fsize + 5);
    stSenderParam[5].cThreadID = 5;

    // Param of thread sender [6]
    stSenderParam[6].nPortNumber = RFTU_PORT_6;
    stSenderParam[6].unWindowSize = RFTU_WINDOW_SIZE;
    stSenderParam[6].nFilePointerStart = *(fpoint + 6);
    stSenderParam[6].nFileSize = *(fsize + 6);
    stSenderParam[6].cThreadID = 6;

    // Param of thread sender [7]
    stSenderParam[7].nPortNumber = RFTU_PORT_7;
    stSenderParam[7].unWindowSize = RFTU_WINDOW_SIZE;
    stSenderParam[7].nFilePointerStart = *(fpoint + 7);
    stSenderParam[7].nFileSize = *(fsize + 7);
    stSenderParam[7].cThreadID = 7;

    // Configure settings of the receiver address struct
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(RFTU_WELCOME_PORT); //8880
    if (inet_aton(rftu_ip, &receiver_addr.sin_addr) == 0)
    {
        printf("[SENDER Main] ERROR: The address is invalid\n");
        return RFTU_RET_ERROR;
    }
    memset(receiver_addr.sin_zero, '\0', sizeof(receiver_addr.sin_zero));
    socklen = sizeof(receiver_addr);

    // Socket creation
    if ((socket_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("[SENDER Main] ERROR: Socket creation fails\n");
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
        printf("[SENDER Main] Sending INIT message\n");
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
            printf("[SENDER Main] ERROR: Error while waiting for packets\n");
            printf("[SENDER Main] ERROR: %s", strerror(errno));
            close(socket_fd);
            return RFTU_RET_ERROR;
        }
        else if (select_result == 0) // Time out
        {
            error_cnt++;
            if (error_cnt == RFTU_MAX_RETRY)
            {
                printf("[SENDER Main] ERROR: Over the limit of sending times\n");
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
                    printf("[SENDER Main] READY message received\n");
                    rftu_id = rftu_pkg_receive.id;  // Get transmission ID
                    // Threads creation
                    {
                        int a, b, c, d, e, f, g, h;
                        a = pthread_create(&pth[0], NULL, &SENDER_Start, (void *)&stSenderParam[0]);
                        b = pthread_create(&pth[1], NULL, &SENDER_Start, (void *)&stSenderParam[1]);
                        c = pthread_create(&pth[2], NULL, &SENDER_Start, (void *)&stSenderParam[2]);
                        d = pthread_create(&pth[3], NULL, &SENDER_Start, (void *)&stSenderParam[3]);
                        e = pthread_create(&pth[4], NULL, &SENDER_Start, (void *)&stSenderParam[4]);
                        f = pthread_create(&pth[5], NULL, &SENDER_Start, (void *)&stSenderParam[5]);
                        g = pthread_create(&pth[6], NULL, &SENDER_Start, (void *)&stSenderParam[6]);
                        h = pthread_create(&pth[7], NULL, &SENDER_Start, (void *)&stSenderParam[7]);
                        if (!a && !b && !c && !d && !e && !f && !g && !h)
                        {
                            if (flag_verbose)
                            {
                                printf("[SENDER Main] Thread Created.\n");
                            }
                            pthread_join(pth[0], NULL);
                            pthread_join(pth[1], NULL);
                            pthread_join(pth[2], NULL);
                            pthread_join(pth[3], NULL);
                            pthread_join(pth[4], NULL);
                            pthread_join(pth[5], NULL);
                            pthread_join(pth[6], NULL);
                            pthread_join(pth[7], NULL);
                            if (flag_verbose)
                            {
                                printf("[SENDER Main] Thread functions are terminated.\n");
                            }
                        }
                        else
                        {
                            if (flag_verbose)
                            {
                                printf("[SENDER Main] ERROR: Thread creation failed.\n");
                            }
                        }
                    }
                    close(socket_fd);
                    return RFTU_RET_OK;
                case RFTU_CMD_NOSPACE:
                    printf("[SENDER Main] ERROR: No available space at receiver machine\n");
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
