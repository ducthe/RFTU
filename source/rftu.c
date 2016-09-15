/*
 * Filename: main.c
 * Author: Richard + Peter
 * Date: 06-Sep-2016
 *
 *
*/
/*************************************************************************/
/* Include file */
#include "rftu.h"
/*Defined*/

unsigned char flag_verbose = NO;
unsigned char flag_server = NO;
unsigned char flag_file_ok = NO;
unsigned char flag_ip_ok = NO;
unsigned char flag_ACK_dropper = NO;
unsigned char flag_Packet_dropper = NO;

unsigned char ACK_loss_probability;
unsigned char Packet_loss_probability;

char    rftu_filename[256];
char    rftu_ip[20];

static int option;
int main(int argc, char *argv[])
{
    while((option = getopt(argc, argv, "f:t:se:vh")) != -1)
    {
        switch(option)
        {
            case 'f':
                if(MAIN_check_file_exist(optarg) == YES)
                {
                    flag_file_ok = YES;
                    memset(rftu_filename, '\0', strlen(rftu_filename));
                    strcpy(rftu_filename, optarg);
                }
                else
                {
                    printf("%s\n", "Error: File doesn't exist!!!!");
                    MAIN_disp_help();
                    return RFTU_RET_ERROR;
                }
                break;
            case 't':
                if(MAIN_check_ip(optarg) == YES)
                {
                    flag_ip_ok = YES;
                    memset(rftu_ip, '\0', strlen(rftu_ip));
                    strcpy(rftu_ip, optarg);
                }
                else
                {
                    printf("%s\n", "Error: IP is wrong!!!");
                    MAIN_disp_help();
                    return RFTU_RET_ERROR;
                }
                break;
            case 's':
                flag_server = YES;
                break;
            case 'e':
                if(flag_server == YES)
                {
                    flag_ACK_dropper = YES;
                    ACK_loss_probability = (unsigned char) atoi(optarg);
                    printf("ACK_loss_probability = %d\n", ACK_loss_probability);
                }
                else
                {
                    flag_Packet_dropper = YES;
                    Packet_loss_probability = (unsigned char) atoi(optarg);
                    printf("Packet_loss_probability = %d\n", Packet_loss_probability);
                }
                break;
            case 'v':
                flag_verbose = YES;
                break;
            case 'h':
                MAIN_disp_help();
                break;
            default:
                MAIN_disp_help();
                return RFTU_RET_ERROR;
        }
    }

    if((flag_file_ok == YES) && (flag_ip_ok == YES))
    {
        printf("[RFTU] Verbose Mode: %s\n", (flag_verbose   == YES ? "ON" : "OFF"));
        printf("[RFTU] Destination IP: %s\n\n", rftu_ip);
        SENDER_Main();
    }

    if(flag_server == YES)
    {
        RECEIVER_Main();
    }
    return RFTU_RET_OK;
}

unsigned char MAIN_check_ip(char *path)
{
    unsigned int addr[4];
    int i;
    int c = sscanf(path, "%3u.%3u.%3u.%3u", &addr[0], &addr[1], &addr[2], &addr[3]);
    if(c != 4)
    {
        return NO;
    }
    for(i = 0; i < 4; i++)
    {
        if((addr[i] < 0) || (addr[i] > 255))
        {
            return NO;
        }
    }
    return YES;
}

unsigned char MAIN_check_file_exist(char *path)
{
    FILE *file;
    if(file = fopen(path, "r"));
    {
        fclose(file);
        return YES;
    }
    return NO;
}

void MAIN_disp_help(void)
{
    printf("\n%s\n\n", "\
        Here are instructions for you\n\n\n\
        Show help information:\n\
        rftu -h\n\
        -----------------------------------------------------------------\n\
        \n\
        Firstly, the receiver is initialized by command:\n\
        rftu -s [-e ACK_loss_probability] [-v]\n\n\
        \n\
        Secondly, the sender is initialized by command:\n\
        rftu -f /path/filename -t destination [-e Packet_loss_probability] [-v]\n\n\
        -----------------------------------------------------------------\n\
        -----------------------------------------------------------------\n\
        \n\
        -s: Run as server mode. Files will be overwriten if existed\n\n\
        -f: path to file that will be sent, must have -t option\n\n\
        -t: IP of destination address, in format X.X.X.X, in pair with -f option\n\n\
        -v: show progress information, this will slow down the progress\n\n\
        -e: Set ACK or packet loss");
}

void MAIN_div_file(unsigned long int filesize, unsigned long int *fsize, unsigned long int *fpoint)
{
    unsigned long int n;
    n = filesize/RFTU_FRAME_SIZE;
    n = n/16*RFTU_FRAME_SIZE;

    *(fpoint + 0) = 0;
    *(fsize + 0) = n;

    *(fpoint + 1) = n;
    *(fsize + 1) = n;

    *(fpoint + 2) = 2*n;
    *(fsize + 2) = n;

    *(fpoint + 3) = 3*n;
    *(fsize + 3) = n;

    *(fpoint + 4) = n*4;
    *(fsize + 4) = n*2;

    *(fpoint + 5) = n*6;
    *(fsize + 5) = n*2;

    *(fpoint + 6) = n*8;
    *(fsize + 6) = n*4;

    *(fpoint + 7) = n*12;
    *(fsize + 7) = filesize - (n*12);
}
