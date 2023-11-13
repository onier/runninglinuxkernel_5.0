#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include "mychardev.h"
//sudo mknod /dev/testchardev c 247 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Hello World kernel module");
#define DEVICE_NAME "testchardev"
static int major_number;
static char *g_testchardev_buffer;
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
#define BUFFER_SIZE 1024

static LIST_HEAD(all_testchardev_private_data_list);
static spinlock_t my_testchardev_private_data_spinlock;
struct testchardev_private_data {
    struct my_chardev_cmd_setting_data settingData;
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

static void free_testchardev_private_data(struct testchardev_private_data *data) {
    if (data) {
        if (data->readData) {
            if (data->readData->data) {
                kfree(data->readData->data);
            }
            kfree(data->readData);
        }
        kfree(data);
    }
}

static int testchardev_release(struct inode *inode, struct file *filp) {
    if (filp->private_data != NULL) {
        struct testchardev_private_data *current_data, *next_data;
        spin_lock(&my_testchardev_private_data_spinlock);
        list_for_each_entry_safe(current_data, next_data, &all_testchardev_private_data_list, list) {
            if (current_data == filp->private_data) {
                list_del(&current_data->list);
                free_testchardev_private_data(current_data);
                filp->private_data = NULL;
            }
        }
        spin_unlock(&my_testchardev_private_data_spinlock);
    }
    return 0;
}

static long testchardev_ioctl_cmd_read_size(struct file *filp, unsigned long arg) {
    if (filp->private_data != NULL) {
        struct testchardev_private_data *data = filp->private_data;
        struct my_chardev_cmd_read_data *sendData = data->readData;
        if (data->readData) {
            if (copy_to_user((int *__user) arg, &data->readData->len,
                             sizeof(int))) {
                return -EFAULT;
            }
            return 0;
        }
    }
    return -EFAULT;
}

static long testchardev_ioctl_cmd_read(struct file *filp, unsigned long arg) {
    if (filp->private_data != NULL) {
        struct testchardev_private_data *data = filp->private_data;
        struct my_chardev_cmd_read_data *sendData = data->readData;
        if (data->readData) {
            struct my_chardev_cmd_read_data readData;
            if (copy_from_user(&readData, (struct my_chardev_cmd_read_data __user *) arg,
                               sizeof(struct my_chardev_cmd_read_data))) {
                return -EFAULT;
            }
            strcpy(readData.from, data->readData->from);
            readData.len = data->readData->len;
            if (copy_to_user((struct my_chardev_cmd_read_data *__user) arg, &readData,
                             sizeof(struct my_chardev_cmd_read_data))) {
                return -EFAULT;
            }
            if (copy_to_user(readData.data, data->readData->data, data->readData->len)) {
                return -EFAULT;
            }
            kfree(data->readData);
            data->readData = NULL;
            return 0;
        }
    }
    return -EFAULT;
}

static long testchardev_ioctl_cmd_send(struct file *filp, unsigned long arg) {
    if (filp->private_data != NULL) {
        struct testchardev_private_data *from = filp->private_data;
        struct my_chardev_cmd_send_data *sendData = kmalloc(sizeof(struct my_chardev_cmd_send_data), GFP_KERNEL);
        if (copy_from_user(sendData, (struct my_chardev_cmd_send_data __user *) arg,
                           sizeof(struct my_chardev_cmd_send_data))) {
            pr_err("copy my_chardev_cmd_send_data send");
            return -EFAULT;
        }
        pr_info("send data to %s len:%d data:%s", sendData->name, sendData->len, sendData->data);
        struct testchardev_private_data *current_data, *next_data;
        spin_lock(&my_testchardev_private_data_spinlock);
        list_for_each_entry_safe(current_data, next_data, &all_testchardev_private_data_list, list) {
            if (strcmp(current_data->settingData.name, sendData->name) == 0) {
                if (current_data->readData == NULL) {
                    current_data->readData = kmalloc(sizeof(struct my_chardev_cmd_read_data), GFP_KERNEL);
                }
                memcpy(current_data->readData->from, from->settingData.name, sizeof(from->settingData.name));
                current_data->readData->len = sendData->len;
                current_data->readData->data = kmalloc(current_data->readData->len, GFP_KERNEL);
                memcpy(current_data->readData->data, sendData->data, current_data->readData->len);
                wake_up_locked_poll(&wait_queue,POLLIN);
            }
        }
        spin_unlock(&my_testchardev_private_data_spinlock);
//        kfree(sendData->data);
        kfree(sendData);
        return 0;
    }
    return -EFAULT;
}

static long testchardev_ioctl_cmd_set(struct file *filp, unsigned long arg) {
    if (filp->private_data == NULL) {
        struct testchardev_private_data *data = kmalloc(sizeof(struct testchardev_private_data), GFP_KERNEL);
        if (copy_from_user(&data->settingData, (struct my_chardev_cmd_setting_data __user *) arg,
                           sizeof(struct my_chardev_cmd_setting_data))) {
            pr_err("Failed to copy data from user space\n");
            return -EFAULT;
        }
        spin_lock_init(&data->lock);
        data->readData = NULL;
        spin_lock(&my_testchardev_private_data_spinlock);
        list_add_tail(&data->list, &all_testchardev_private_data_list);
        spin_unlock(&my_testchardev_private_data_spinlock);
        filp->private_data = data;
        return 0;
    } else {
        printk(KERN_ERR, "the testchardev_ioctl_cmd_set was set");
    }
    return -EFAULT;
}

static long testchardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case MY_CUSTOM_IOCTL_CMD_SETTING:
            return testchardev_ioctl_cmd_set(filp, arg);
        case MY_CUSTOM_IOCTL_CMD_READ:
            return testchardev_ioctl_cmd_read(filp, arg);
        case MY_CUSTOM_IOCTL_CMD_SEND:
            return testchardev_ioctl_cmd_send(filp, arg);
        case MY_CUSTOM_IOCTL_CMD_READ_SIZE:
            return testchardev_ioctl_cmd_read_size(filp, arg);
        default:
            return -EINVAL;
    }
    return 0;
}

static __poll_t testchardev_poll(struct file *filp, poll_table *wait) {
    unsigned int mask = 0;
    poll_wait(filp, &wait_queue, wait);
    struct testchardev_private_data *data = filp->private_data;
    if (data && data->readData) {
        mask |= POLLIN | POLLRDNORM;  // 数据可读事件
    }
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
