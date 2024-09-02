#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("danilka108");
MODULE_DESCRIPTION("Danil Churikov eltex summer module 5 lab 2");

#define MSG_SIZE 16
static char msg[MSG_SIZE];

static struct proc_dir_entry *entry;

ssize_t proc_read(struct file *filp, char __user *buff, size_t buff_size, loff_t *offset);
ssize_t proc_write(struct file *filp, const char __user *buff, size_t buff_size, loff_t *offset);

static const struct proc_ops ops = {
  proc_read: proc_read,
  proc_write: proc_write,
};

ssize_t proc_read(struct file *filp, char __user *buff, size_t buff_size, loff_t *offset) {
  if (*offset >= MSG_SIZE) {
    return 0;
  }

  size_t len = buff_size < MSG_SIZE - *offset ? buff_size : MSG_SIZE - *offset;
  if (copy_to_user(buff, msg + *offset, len)) {
    pr_err("failed to copy to user\n");
    return -EFAULT;
  }

  *offset += len;

  pr_info("successfully copied %zu bytes to user\n", len);
  return len;
}

ssize_t proc_write(struct file *filp, const char __user *buff, size_t buff_size, loff_t *offset) {
  if (*offset >= MSG_SIZE) {
    return 0;
  }

  size_t len = buff_size < MSG_SIZE - *offset ? buff_size : MSG_SIZE - *offset;
  if (copy_from_user(msg + *offset, buff, len)) {
    pr_err("failed to copy from user\n");
    return -EFAULT;
  }

  *offset += len;

  pr_info("successfully copied %zu bytes from user\n", len);
  return len;
}

static __init int lab2_module_init(void) {
  entry = proc_create("lab2_module", S_IRWXU | S_IRWXG | S_IRWXO, NULL, &ops);
  return 0;
}

static __exit void lab2_module_cleanup(void) {
  remove_proc_entry("lab2_module", entry);
}

module_init(lab2_module_init);
module_exit(lab2_module_cleanup);
