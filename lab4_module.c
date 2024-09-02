#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/irq.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/poll.h> 
#include <linux/atomic.h>

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define MSG_SIZE 128

enum {
  LAB4_CDEV_BUSY,
  LAB4_CDEV_FREE,
};

static atomic_t opening_status = ATOMIC_INIT(LAB4_CDEV_FREE);
static int major;
static struct class *cls;
static char msg[MSG_SIZE];

static int lab4_init_module(void);
static void lab4_cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static const struct file_operations fops = {
  .open =  device_open,
  .release =  device_release,
  .read =  device_read,
  .write =  device_write,
};

static int __init lab4_init_module(void) {
  major = register_chrdev(0, DEVICE_NAME, &fops);
  if (major < 0) {
    pr_alert("failed to register char device: %d\n", major);
    return major;
  }

  /*cls = class_create(THIS_MODULE, DEVICE_NAME);*/
  cls = class_create(DEVICE_NAME);
  device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

  pr_info("char device %d,%d created on /dev/%s\n", major, 0, DEVICE_NAME);

  return SUCCESS;
}

static void __exit lab4_cleanup_module(void) {
  device_destroy(cls, MKDEV(major, 0));
  class_destroy(cls);

  unregister_chrdev(major, DEVICE_NAME);
}

static int device_open(struct inode* inode, struct file *filp) {
  static int openings_number = 0;

  if (atomic_cmpxchg(&opening_status, LAB4_CDEV_FREE, LAB4_CDEV_BUSY) != LAB4_CDEV_FREE) {
    return -EBUSY;
  }
  
  snprintf(msg, MSG_SIZE, "I already told you %d times Hello world!\n", openings_number++);
  if (!try_module_get(THIS_MODULE)) {
    return -EFAULT;
  }

  return SUCCESS;
}

static int device_release(struct inode * inode, struct file * filp) {
  atomic_set(&opening_status, LAB4_CDEV_FREE);
  module_put(THIS_MODULE);

  return SUCCESS;
}


static ssize_t device_read(struct file *filp, char *buff, size_t buff_size, loff_t *offset) {
  if (*offset >= MSG_SIZE) {
    *offset = 0;
    return 0;
  }

  size_t len = buff_size < MSG_SIZE - *offset ? buff_size : MSG_SIZE - *offset;
  if (copy_to_user(buff, msg + *offset, len)) {
    return -EFAULT;
  }

  *offset += len;
  return len;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t buff_size, loff_t *offset) {
  pr_alert("Sorry, this operation is not supported.\n"); 
  return -EINVAL;
}

module_init(lab4_init_module);
module_exit(lab4_cleanup_module);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Danil Churikov eltex summer module 5 lab 4");
