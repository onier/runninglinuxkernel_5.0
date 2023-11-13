#include <linux/mm.h>
#include "linux/module.h"
#include "linux/slab.h"
#include "linux/fs.h"

#define DEVICE_NAME "my_mmap_device"
static int major_number;

struct my_mmap_data {
    char *buffer;
};

#define SHARED_MEMORY_SIZE 4096*4

static int my_map_device_open(struct inode *inode, struct file *filp) {
    struct my_mmap_data *data;

    data = kmalloc(sizeof(struct my_mmap_data), GFP_KERNEL);
    if (!data) {
        pr_err("Failed to allocate memory for private data\n");
        return -ENOMEM;
    }
    data->buffer = kmalloc(SHARED_MEMORY_SIZE, GFP_KERNEL);
    if (!data->buffer) {
        pr_err("Failed to allocate shared memory\n");
        kfree(data);
        return -ENOMEM;
    }
    filp->private_data = data;
    pr_info("Device opened\n");
    return 0;
}

static int my_map_device_release(struct inode *inode, struct file *filp) {
    struct my_mmap_data *data = filp->private_data;

    // 释放共享内存
    kfree(data->buffer);
    kfree(data);

    pr_info("Device closed\n");

    return 0;
}

static int my_map_device_mmap(struct file *filp, struct vm_area_struct *vma) {
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long pfn;
    struct my_mmap_data *data = filp->private_data;

    if (size > SHARED_MEMORY_SIZE) {
        pr_err("Requested size exceeds the size of shared memory\n");
        return -EINVAL;
    }

    // 转换物理页帧号
    pfn = virt_to_phys(data->buffer) >> PAGE_SHIFT;

    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
        pr_err("Failed to remap the shared memory\n");
        return -EAGAIN;
    }

    pr_info("mmap successful 0x%p-------0x%p\n", vma->vm_start, vma->vm_end);

    return 0;
}

const struct file_operations my_map_device_ops = {
        .owner = THIS_MODULE,
        .open = my_map_device_open,
        .release = my_map_device_release,
        .mmap = my_map_device_mmap,
};

static int __init my_mmap_device_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &my_map_device_ops);
    if (major_number < 0) {
        pr_err("register_chrdev %s fail ", DEVICE_NAME);
        return -EFAULT;
    }
    pr_info("register_chrdev %s sucess major_number:%d", DEVICE_NAME, major_number);
    return 0;
}

static void __exit my_mmap_device_exit(void) {
    unregister_chrdev(major_number, DEVICE_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Hello World kernel module");
module_init(my_mmap_device_init);
module_exit(my_mmap_device_exit);
