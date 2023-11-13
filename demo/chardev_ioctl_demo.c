#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/slab.h>

#define MY_IOCTL_MAGIC 'k'
#define MY_IOCTL_SET_DATA _IOW(MY_IOCTL_MAGIC, 1, struct ioctl_data)
#define MY_IOCTL_GET_DATA _IOR(MY_IOCTL_MAGIC, 2, struct ioctl_data)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");

struct ioctl_data {
    size_t size;
    char *buffer;
};

static dev_t dev;
static struct cdev my_cdev;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("Opened the device\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("Closed the device\n");
    return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ioctl_data user_data;
    char *kernel_buffer;
    int ret = 0;

    switch (cmd) {
        case MY_IOCTL_SET_DATA:
            if (copy_from_user(&user_data, (struct ioctl_data *)arg, sizeof(struct ioctl_data))) {
                ret = -EFAULT;
                break;
            }

            kernel_buffer = kmalloc(user_data.size, GFP_KERNEL);
            if (!kernel_buffer) {
                ret = -ENOMEM;
                break;
            }

            if (copy_from_user(kernel_buffer, user_data.buffer, user_data.size)) {
                kfree(kernel_buffer);
                ret = -EFAULT;
                break;
            }

            // Process the data as needed
            pr_info("Received data in kernel: %s\n", kernel_buffer);

            kfree(kernel_buffer);
            break;

        case MY_IOCTL_GET_DATA:
            // Allocate and prepare kernel data
            kernel_buffer = kmalloc(256, GFP_KERNEL);
            if (!kernel_buffer) {
                ret = -ENOMEM;
                break;
            }
            snprintf(kernel_buffer, 256, "Hello from kernel");

            user_data.size = strlen(kernel_buffer) + 1;
            user_data.buffer = kmalloc(user_data.size, GFP_KERNEL);
            if (!user_data.buffer) {
                kfree(kernel_buffer);
                ret = -ENOMEM;
                break;
            }

            if (copy_to_user((struct ioctl_data *)arg, &user_data, sizeof(struct ioctl_data))) {
                kfree(kernel_buffer);
                kfree(user_data.buffer);
                ret = -EFAULT;
                break;
            }

            kfree(kernel_buffer);
            kfree(user_data.buffer);
            break;

        default:
            ret = -EINVAL;
    }

    return ret;
}

static struct file_operations my_fops = {
        .owner = THIS_MODULE,
        .open = my_open,
        .release = my_release,
        .unlocked_ioctl = my_ioctl,
};

int major_number;

static int __init my_init(void)
{
    major_number = register_chrdev(0, "my_device", &my_fops);
    if (major_number < 0) {
        printk(KERN_ERR       "register_chrdev testchardev fail %d \n", major_number);
        return major_number;
    }
    printk(KERN_INFO       "%s sucess %d!\n", "my_device", major_number);
    return 0;
//    int ret;
//
//    ret = alloc_chrdev_region(&dev, 0, 1, "my_device");
//    if (ret < 0) {
//        pr_err("Failed to allocate char device region\n");
//        return ret;
//    }
//
//    cdev_init(&my_cdev, &my_fops);
//    ret = cdev_add(&my_cdev, dev, 1);
//    if (ret < 0) {
//        pr_err("Failed to add char device\n");
//        unregister_chrdev_region(dev, 1);
//        return ret;
//    }
//
//    pr_info("String Device Driver Initialized\n");
//
//    return 0;
}

static void __exit my_exit(void)
{
//    cdev_del(&my_cdev);
//    unregister_chrdev_region(dev, 1);
    unregister_chrdev(major_number, "my_device");
    printk(KERN_INFO "unregister_chrdev \n");
    pr_info("String Device Driver Uninitialized\n");
}

module_init(my_init);
module_exit(my_exit);
