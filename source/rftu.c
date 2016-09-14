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

char    rftu_filename[256];
char    rftu_ip[20];

static int option;
int main(int argc, char *argv[])
{
    while((option = getopt(argc, argv, "f:t:svh")) != -1)
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
    printf("\n%s\n", "Here are instructions for you\n\n\n\
            rftu [-f /path/filename -t destination] [-s] [-v]\n\n\
            rftu [--help] or [-h]\n\n\
            -s: Run as server mode. Files will be overwriten if existed\n\n\
            -f: path to file that will be sent, must have -t option\n\
            -t: IP of destination address, in format X.X.X.X, in pair with -f option\n\n\
            -v: show progress information, this will slow down the progress\n\n\
            -h of --help: show help information\n\n");
}

void MAIN_div_file(unsigned long int filesize, unsigned long int *fsize)
{
    int n;
    n = filesize/4;

    *(fsize + 0) = n * 1;
    *(fsize + 1) = filesize - n;
}