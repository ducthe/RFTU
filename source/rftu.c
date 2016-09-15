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

unsigned char ucFlagVerbose = NO;
unsigned char ucFlagServer = NO;
unsigned char ucFlagFile = NO;
unsigned char ucFlagIP = NO;
unsigned char ucFlagACKDrop = NO;
unsigned char ucFlagPacketDrop = NO;

unsigned char ucAckLossRate;
unsigned char ucPacketLossRate;

char    cRFTUFileName[256];
char    cRFTUIP[20];

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
                    ucFlagFile = YES;
                    memset(cRFTUFileName, '\0', strlen(cRFTUFileName));
                    strcpy(cRFTUFileName, optarg);
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
                    ucFlagIP = YES;
                    memset(cRFTUIP, '\0', strlen(cRFTUIP));
                    strcpy(cRFTUIP, optarg);
                }
                else
                {
                    printf("%s\n", "Error: IP is wrong!!!");
                    MAIN_disp_help();
                    return RFTU_RET_ERROR;
                }
                break;
            case 's':
                ucFlagServer = YES;
                break;
            case 'e':
                if(ucFlagServer == YES)
                {
                    ucFlagACKDrop = YES;
                    ucAckLossRate = (unsigned char) atoi(optarg);
                    printf("ucAckLossRate = %d\n", ucAckLossRate);
                }
                else
                {
                    ucFlagPacketDrop = YES;
                    ucPacketLossRate = (unsigned char) atoi(optarg);
                    printf("ucPacketLossRate = %d\n", ucPacketLossRate);
                }
                break;
            case 'v':
                ucFlagVerbose = YES;
                break;
            case 'h':
                MAIN_disp_help();
                break;
            default:
                MAIN_disp_help();
                return RFTU_RET_ERROR;
        }
    }

    if((ucFlagFile == YES) && (ucFlagIP == YES))
    {
        printf("[RFTU] Verbose Mode: %s\n", (ucFlagVerbose   == YES ? "ON" : "OFF"));
        printf("[RFTU] Destination IP: %s\n\n", cRFTUIP);
        SENDER_Main();
    }

    if(ucFlagServer == YES)
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
        rftu -s [-e ucAckLossRate] [-v]\n\n\
        \n\
        Secondly, the sender is initialized by command:\n\
        rftu -f /path/filename -t destination [-e ucPacketLossRate] [-v]\n\n\
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
