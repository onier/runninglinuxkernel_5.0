//
// Created by ubuntu on 2023/11/10.
//

#ifndef RUNNINGLINUXKERNEL_5_0_MYCHARDEV_H
#define RUNNINGLINUXKERNEL_5_0_MYCHARDEV_H
// 在头文件中定义 IOCTL 命令和数据结构
#define MY_CUSTOM_IOCTL_CMD_SETTING _IOW('MY_CUSTOM_IOCTL_CMD_SETTING', 1, struct my_chardev_cmd_setting_data)
#define MY_CUSTOM_IOCTL_CMD_REAND _IOW('MY_CHARDEV_CMD_READ', 2, struct my_chardev_cmd_read_data)
#define MY_CUSTOM_IOCTL_CMD_SEND _IOW('MY_CHARDEV_CMD_WRITE', 3, struct my_chardev_cmd_send_data)
#define SETTING_CMD
#define READ_CMD
#define SEND_CMD

struct my_chardev_cmd_setting_data {
    char name[32];
};

struct my_chardev_cmd_read_data {
    int len;
    char *data;
};

struct my_chardev_cmd_send_data {
    char name[32];
    int len;
    char *data;
};

#endif //RUNNINGLINUXKERNEL_5_0_MYCHARDEV_H
