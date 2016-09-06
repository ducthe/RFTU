/*
 * Filename: main.c
 * Author: Henry
 * Contributor: Peter + Richard
 * Date: 01-Sep-2016
 *
 *


*/
/*************************************************************************/
/* Include file */
#include "rftu.h"
/*Defined*/

flag_verbose = NO;
flag_server = NO;
flag_file_ok = NO;
flag_ip_ok = NO;

static int option;
int main()
{
    while(option = getopt(argc, argv, "f:t:sv") != -1)
    {
        switch(option)
        {
            case 'f':
                if(check_file_exist(optarg) == YES)
                {
                    flag_file_ok = YES;
                    strcp(rftu_filename, optarg);
                }
                else
                {
                    printf("%s\n", "Error: File doesn't exist!!!!");
                    return RFTU_RET_ERROR;
                }
                break;
            case 't':
                if(check_ip(optarg) == YES)
                {
                    flag_ip_ok = YES;
                    strcp(rftu_ip, optarg);
                }
                else
                {
                    printf("%s\n", "Error: IP is wrong!!!");
                    return RFTU_RET_ERROR;
                }
                break;
            case 's':
                flag_server = YES;
                break;
            case 'v':
                flag_verbose = YES;
                break;
            default:
                disp_help();
                return RFTU_RET_ERROR;
        }
    }
    while(option = getopt_long(argv, argc, "h", long_options, &option_index) != -1)
    {
        if(option == 'h')
        {
            disp_help();
            return RFTU_RET_OK;
        }
    }

    if((flag_file_ok == YES) && (flag_ip_ok == YES))
    {
        rftu_sender();
    }
    if(flag_server == YES)
    {
        rftu_receiver();
    }
    else
    {
        disp_help();
    }
    return RFTU_RET_OK;
}

unsigned char check_ip(char *path)
{
    unsigned int addr[4];
    int i;
    int c = sscanf(path, %3u.%3u.%3u.%3u, &addr[0], &addr[1], &addr[2], &addr[3]);
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

unsigned char check_file_exist(char *path)
{
    FILE *file;
    if(file = fopen(path, "r"));
    {
        fclose(file);
        return YES;
    }
    return NO;
}

void disp_help(void)
{
    printf("\n%s\n", "This is help for you\
            rftu [-f /path/filename -t destination] [-s] [-v] [-h]\n\n\
            -s: Run as server mode. Files will be overwriten if existed\n\n\
            -f: path to file that will be sent, must have -t option\n\
            -t: IP of destination address, in format X.X.X.X, in pair with -f option\n\n\
            -v: show progress information, this will slow down the progress\n\n\
            -h: show help information\n\n");
}
