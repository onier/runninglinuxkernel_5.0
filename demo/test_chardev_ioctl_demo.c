#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

#define MY_IOCTL_MAGIC 'k'
#define MY_IOCTL_SET_DATA _IOW(MY_IOCTL_MAGIC, 1, struct ioctl_data)
#define MY_IOCTL_GET_DATA _IOR(MY_IOCTL_MAGIC, 2, struct ioctl_data)

struct ioctl_data {
    size_t size;
    char *buffer;
};

int main()
{
    int fd = open("/dev/my_device", O_RDWR);

    if (fd == -1)
    {
        perror("Error opening device");
        return -1;
    }

    // Example of setting data from user space
//    char write_buffer[] = "Data from user space";
    char * write_buffer = "Data from user space";
    struct ioctl_data set_data;
    set_data.size = strlen(write_buffer);
    set_data.buffer = write_buffer;

    if (ioctl(fd, MY_IOCTL_SET_DATA, &set_data) == -1)
    {
        perror("Error setting data via ioctl");
        close(fd);
        return -1;
    }

    // Example of getting data from kernel space
    struct ioctl_data get_data;
    if (ioctl(fd, MY_IOCTL_GET_DATA, &get_data) == -1)
    {
        perror("Error getting data via ioctl");
        close(fd);
        return -1;
    }

    printf("Data received from kernel: %s\n", get_data.buffer);

    // Cleanup
    free(get_data.buffer);
    close(fd);

    return 0;
}
