#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include "mychardev.h"
//sudo mknod /dev/mychardev c 247 0
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Hello World kernel module");
#define DEVICE_NAME "testchardev"
static int major_number;
static char *g_testchardev_buffer;
static struct poll_table_struct wait_queue;
#define BUFFER_SIZE 1024

static LIST_HEAD(all_testchardev_private_data_list);
static spinlock_t my_testchardev_private_data_spinlock;
struct testchardev_private_data {
    struct my_chardev_cmd_setting_data settingData;
    char from[32];
    struct my_chardev_cmd_read_data *readData;
    spinlock_t lock;
    struct list_head list;  // 内核链表节点
};

static int testchardev_open(struct inode *inode, struct file *f) {
    return 0;
}

static ssize_t testchardev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
    struct task_struct *current_task = current;
    int bytes_to_copy = min(length, BUFFER_SIZE - *offset);
    if (copy_to_user(buffer, g_testchardev_buffer + *offset, bytes_to_copy) != 0) {
        return -EFAULT;
    }
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t testchardev_write(struct file *filp, const char *buffer, size_t length, loff_t *offset) {
    struct task_struct *current_task = current;
    printk(KERN_INFO       "pid:%d tgid:%d name:%s buffer:%s parent_id:%d parent_tgid:%d parent_name:%s buffer:%s ",
           current->pid, current->tgid, current->comm,
           current->parent->pid, current->parent->tgid, current->parent->comm, buffer);
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
    if (filp->private_data != NULL) {
        kfree(filp->private_data);
    }
    return 0;
}

static long testchardev_ioctl_cmd_read(struct file *filp, unsigned long arg) {
    if (filp->private_data != NULL) {

    }
    return -EFAULT;
}

static long testchardev_ioctl_cmd_send(struct file *filp, unsigned long arg) {
    if (filp->private_data != NULL) {

        struct testchardev_private_data *current_data, *next_data;
        spin_lock(&my_testchardev_private_data_spinlock);
        list_for_each_entry_safe(current_data, next_data, &all_testchardev_private_data_list, list) {

        }
    }
    return -EFAULT;
}

static long testchardev_ioctl_cmd_set(struct file *filp, unsigned long arg) {
    if (filp->private_data == NULL) {
        struct testchardev_private_data *data = kmalloc(sizeof(struct testchardev_private_data), GFP_KERNEL);
        spin_lock_init(&data->lock);
        if (copy_from_user(&data->settingData, (struct my_chardev_cmd_setting_data __user *) arg,
                           sizeof(struct my_chardev_cmd_setting_data))) {
            pr_err("Failed to copy data from user space\n");
            return -EFAULT;
        }
        spin_lock(&my_testchardev_private_data_spinlock);
        list_add_tail(&data->list, &all_testchardev_private_data_list);
        spin_unlock(&my_testchardev_private_data_spinlock);
        filp->private_data = data;
    } else {
        printk(KERN_ERR, "the testchardev_ioctl_cmd_set was set");
    }
    return -EFAULT;
}

static long testchardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case MY_CUSTOM_IOCTL_CMD_SETTING:
            return testchardev_ioctl_cmd_set(filp, arg);
        case MY_CUSTOM_IOCTL_CMD_REAND:
            return testchardev_ioctl_cmd_read(filp, arg);
        case MY_CUSTOM_IOCTL_CMD_SEND:
            return testchardev_ioctl_cmd_set(filp, arg);
        default:
            return -EINVAL;
    }
    return 0;
}

static __poll_t testchardev_poll(struct file *filp, struct poll_table_struct *wait) {
    unsigned int mask = 0;

    poll_wait(filp, &wait_queue, wait);

//    // 根据设备状态设置事件掩码
//    if (buffer_len > 0) {
//        mask |= POLLIN | POLLRDNORM;  // 数据可读事件
//    }

    return mask;
}

struct file_operations mychardev_fops = {
        .owner = THIS_MODULE,
        .open = testchardev_open,
        .read  = testchardev_read,
        .write = testchardev_write,
        .release = testchardev_release,
        .poll = testchardev_poll,
        .unlocked_ioctl = testchardev_ioctl,
};

static int __init hello_init(void) {
    spin_lock_init(&my_testchardev_private_data_spinlock);
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
    kfree(g_testchardev_buffer);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "unregister_chrdev \n");
}

module_init(hello_init);
module_exit(hello_exit);
