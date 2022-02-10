#include <stdio.h>      // printf
#include <malloc.h>     // self explanatory
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, access
#include <string.h>     // strlen
#include <stdlib.h>     // exit

#define DEVICE "/dev/my_device_driver"
#define BUFFER_SIZE 3

int fd;

int write_device(void)
{
    int write_len = 0;
    ssize_t ret;

    char *data = (char *)malloc(BUFFER_SIZE * sizeof(char));

    printf("please enter the data to write into the device\n");
    scanf(" %[^\n]", data); // space is added to read white space and stops until newline

    write_len = strlen(data);

    ret = write(fd, data, write_len);
    if (ret == -1)
        printf("writing failed\n");
    else
        printf("writing succeeded\n");
    
    free(data);
    return 0;
};

int read_device(void)
{
    int read_len = 0;
    ssize_t ret;

    char *data = (char *)malloc(BUFFER_SIZE * sizeof(char));

    printf("please enter the length of data to read from device\n");
    scanf("%d", &read_len);

    memset(data, 0, sizeof(data));
    data[0] = '\0';
    // manually change this offset in user space
    lseek(fd, 0, SEEK_SET);
    ret = read(fd, data, read_len);
    if (ret == -1)
        printf("reading failed\n");
    else
        printf("reading succeeded\n");
    printf("Device read: %s\n", data);
    return 0;
};

int main(void)
{   
    char option;

    if (access(DEVICE, F_OK) == -1) {
        printf("module %s is not loaded\n", DEVICE);
        return 0;
    } else
        printf("module %s loaded, will be used\n", DEVICE);

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        printf("Cannot open device file...\n");
        return 0;
    }

    while(1) {
                printf("****Please Enter the Option******\n");
                printf("        1. Write               \n");
                printf("        2. Read                 \n");
                printf("        3. Exit                 \n");
                printf("*********************************\n");
                scanf(" %c", &option);
                printf("Your Option = %c\n", option);
                
                switch(option) {
                        case '1':
                                printf("Data Writing ...\n");
                                write_device();
                                printf("Done!\n");
                                break;
                        case '2':
                                printf("Data Reading ...\n");
                                read_device();
                                printf("Done!\n");
                                break;
                        case '3':
                                close(fd);
                                exit(1);
                                break;
                        default:
                                printf("Enter Valid option = %c\n",option);
                                break;
                }
    }
    close(fd);
    return 0;
};
