#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/my_mmap_device"
#define SHARED_MEMORY_SIZE (4096 * 4)  // 4 pages

int main() {
    int fd;
    char *mapped_buffer;

    // 打开设备文件
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("Failed to open device file");
        return EXIT_FAILURE;
    }

    // 进行 mmap 操作
    mapped_buffer = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_buffer == MAP_FAILED) {
        perror("Failed to map shared memory");
        close(fd);
        return EXIT_FAILURE;
    }

    // 读写共享内存
    printf("Writing to shared memory...\n");
    snprintf(mapped_buffer, SHARED_MEMORY_SIZE, "Hello from user space!");

    printf("Reading from shared memory: %s\n", mapped_buffer);

    // 解除映射
    if (munmap(mapped_buffer, SHARED_MEMORY_SIZE) == -1) {
        perror("Failed to unmap shared memory");
    }

    // 关闭设备文件
    if (close(fd) == -1) {
        perror("Failed to close device file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
