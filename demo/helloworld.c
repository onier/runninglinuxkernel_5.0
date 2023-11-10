#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Hello World kernel module");
#define DEVICE_NAME "testchardev"
static int major_number;
static char *g_testchardev_buffer;
#define BUFFER_SIZE 1024

static int testchardev_open(struct inode *inode, struct file *f) {
    return 0;
}

static ssize_t testchardev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
    int bytes_to_copy = min(length, BUFFER_SIZE - *offset);
    if (copy_to_user(g_testchardev_buffer, g_testchardev_buffer + *offset, bytes_to_copy) != 0) {
        return -EFAULT;
    }
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t testchardev_write(struct file *filp, const char *buffer, size_t length, loff_t *offset) {
    int bytes_to_copy = min(length, BUFFER_SIZE - *offset);
    if (bytes_to_copy <= 0) {
        return -ENOSPC; // No space left on device
    }
    if (copy_from_user(g_testchardev_buffer + *offset, buffer, bytes_to_copy) != 0) {
        return -EFAULT; // Error copying data from user space
    }
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static int testchardev_release(struct inode *inode, struct file *filp) {
    return 0;
}

struct file_operations mychardev_fops = {
        .owner = THIS_MODULE,
        .open = testchardev_open,
        .read  = testchardev_read,
        .write = testchardev_write,
        .release = testchardev_release,
};

static int __init hello_init(void) {
    g_testchardev_buffer = kmalloc(1024, GFP_KERNEL);
    major_number = register_chrdev(0, DEVICE_NAME, &mychardev_fops);
    if (major_number < 0) {
        printk(KERN_ERR       "register_chrdev testchardev fail %d \n", major_number);
        return major_number;
    }
    printk(KERN_INFO       "%s sucess %d!\n", DEVICE_NAME, major_number);
    return 0;
}

static void __exit hello_exit(void) {
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "unregister_chrdev \n");
}

module_init(hello_init);
module_exit(hello_exit);
