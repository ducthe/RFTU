/*
*   Filename    : rftu.c
*   Author      : Richard, Peter (OCTO Team)
*   Contributor : Kevin
*   Date        : 06-Sep-2016
*/

/* Include file */
#include "rftu.h"

unsigned char ucFlagVerbose = NO;
unsigned char ucFlagServer = NO;
unsigned char ucFlagFile = NO;
unsigned char ucFlagIP = NO;
unsigned char ucFlagACKDrop = NO;
unsigned char ucFlagPacketDrop = NO;

float fAckLossRate;
float fPacketLossRate;

unsigned int unThreadNumber;

char    cRFTUFileName[256];
char    cRFTUIP[20];

static int s_nOption;
int main(int argc, char *argv[])
{
    unThreadNumber = 1;
    while((s_nOption = getopt(argc, argv, "f:t:sn:e:vh")) != -1)
    {
        switch(s_nOption)
        {
            case 'f':
                if(MAIN_CheckFileExist(optarg) == RFTU_RET_OK)
                {
                    ucFlagFile = YES;
                    memset(cRFTUFileName, '\0', strlen(cRFTUFileName));
                    strcpy(cRFTUFileName, optarg);
                }
                else
                {
                    printf("%s\n", "[RFTU]ERROR: File doesn't exist!");
                    MAIN_DispHelp();
                    return RFTU_RET_ERROR;
                }
                break;
            case 't':
                if(MAIN_CheckIP(optarg) == YES)
                {
                    ucFlagIP = YES;
                    memset(cRFTUIP, '\0', strlen(cRFTUIP));
                    strcpy(cRFTUIP, optarg);
                }
                else
                {
                    printf("%s\n", "[RFTU] ERROR: Wrong IP!");
                    MAIN_DispHelp();
                    return RFTU_RET_ERROR;
                }
                break;
            case 's':
                ucFlagServer = YES;
                break;
            case 'n':
                if (ucFlagServer)
                {
                    unThreadNumber = (unsigned char) atoi(optarg);
                }
                if (unThreadNumber > THREAD_NUMBER)
                {
                    printf("[RFTU] ERROR: The desired thread numbers is bigger than the maximum threads available to use.\n");
                    return RFTU_RET_ERROR;
                }
                break;
            case 'e':
                if(ucFlagServer == YES)
                {
                    ucFlagACKDrop = YES;
                    fAckLossRate = atof(optarg);
                    printf("[RFTU] ACK loss rate  = %.2f\n", fAckLossRate);
                }
                else
                {
                    ucFlagPacketDrop = YES;
                    fPacketLossRate = atof(optarg);
                    printf("[RFTU] Packet loss rate = %.2f\n", fPacketLossRate);
                }
                break;
            case 'v':
                ucFlagVerbose = YES;
                break;
            case 'h':
                MAIN_DispHelp();
                break;
            default:
                MAIN_DispHelp();
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

unsigned char MAIN_CheckIP(char *path)
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

unsigned char MAIN_CheckFileExist(char *path)
{
    if ( access( path, F_OK | R_OK) != -1 )
    {
        /* File exists */
        return RFTU_RET_OK;
    }
    else
    {
        /* File doesn't exist */
        return RFTU_RET_ERROR;
    }
}

void MAIN_DispHelp(void)
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

void MAIN_FileDiv(unsigned long int filesize, unsigned long int *fsize, unsigned long int *fpoint, unsigned char ucPort)
{
    int i;
    unsigned long int n;
    n = filesize/RFTU_FRAME_SIZE;
    n = n/ucPort*RFTU_FRAME_SIZE;
    for (i = 0; i < ucPort; i++)
    {
        *(fpoint + i) = n*i;
        if (i < ucPort - 1)
        {
            *(fsize + i) = n;
        }
        else
        {
            *(fsize + i) = filesize - *(fpoint + i);
        }
    }
}
