#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>

#define DEVICE_NAME "dummy-dmesg"
#define BUF_LEN 1000

static int major;
static char msg[BUF_LEN];
static DEFINE_MUTEX(msg_mutex);

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    ssize_t ret;

    if (len > BUF_LEN - 1)
        len = BUF_LEN - 1;

    if (!mutex_trylock(&msg_mutex)) {
        return -EBUSY;
    }

    if (copy_from_user(msg, buf, len)) {
        mutex_unlock(&msg_mutex);
        return -EFAULT;
    }

    msg[len] = '\0';
    printk(KERN_INFO "%s\n", msg);

    ret = len;
    mutex_unlock(&msg_mutex);

    return ret;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = dev_write,
};

static int __init dummy_dmesg_device_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Registering dummy device failed with %d\n", major);
        return major;
    }
    printk(KERN_INFO "module loaded with device major number %d\n", major);
    return 0;
}

static void __exit dummy_dmesg_device_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "char_device module unloaded\n");
}

module_init(dummy_dmesg_device_init);
module_exit(dummy_dmesg_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hitoshi Mitake");
MODULE_DESCRIPTION("char device for printing dummy message to kernel log");
