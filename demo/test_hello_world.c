#include <stdio.h>
#include "mychardev.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

int set_name(char *nameStr, int fd) {
    struct my_chardev_cmd_setting_data set_data;
    strcpy(set_data.name, nameStr);
    if (ioctl(fd, MY_CUSTOM_IOCTL_CMD_SETTING, &set_data) == -1) {
        perror("Error setting data via ioctl");
        return -1;
    }
    return 0;
}

int send_data(char *data, int len, char *to, int fd) {
    struct my_chardev_cmd_send_data sendData;
    strcpy(sendData.name, to);
    sendData.len = len;
//    memcpy(sendData.data, data, len);
    sendData.data = data;
    sendData.len = len;
    if (ioctl(fd, MY_CUSTOM_IOCTL_CMD_SEND, &sendData) == -1) {
        perror("Error setting data via ioctl");
        return -1;
    }
    return 0;
}

int read_data(int fd) {
    struct my_chardev_cmd_read_data get_data;
    if (ioctl(fd, MY_CUSTOM_IOCTL_CMD_REAND, &get_data) == -1) {
        perror("Error getting data via ioctl");
        return -1;
    }
    printf("read data %s %d %s \n", get_data.from, get_data.len, get_data.data);
    return 0;
}

int test_poll(int fd) {
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLRDNORM;

    printf("Waiting for data from the device...\n");

    int ret = poll(fds, 1, -1);  // Blocking wait for data

    if (ret == -1) {
        perror("Error in poll");
        return -1;
    }

    if (fds[0].revents & POLLIN) {
        // Data is available for reading
        read_data(fd);
    } else {
        printf("No data available.\n");
    }
}

int main(int argc, char **argvs) {
//        xxx  name1 abc123 name2
    printf("read test start \n");
    int fd = open("/dev/testchardev", O_RDWR);

    if (fd == -1) {
        perror("Error opening device");
        return -1;
    }
    set_name(argvs[1], fd);
    send_data(argvs[2], strlen(argvs[2]), argvs[3], fd);
    test_poll(fd);

    return 0;
}
