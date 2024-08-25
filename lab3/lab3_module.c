#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>

static struct tty_driver *lab3_tty_driver;
static unsigned int switcher;

static ssize_t switcher_show(struct kobject *kobj, struct kobj_attribute *attr, char *buff) {
    return sprintf(buff, "%d\n", switcher);
}

static ssize_t switcher_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t count) {
    if (sscanf(buff, "%du", &switcher) != 1) {
        return -EINVAL;
    }

    (lab3_tty_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, switcher);

    return count;
}

static struct kobject *lab3_kobj;

static struct kobj_attribute switcher_attr = __ATTR(switcher, 0660, switcher_show, switcher_store);

static int __init lab3_init(void) {
    printk(KERN_INFO "lab3: loading\n");
    printk(KERN_INFO "lab3: fgconsole is %x\n", fg_console);

    for (int i = 0; i < MAX_NR_CONSOLES; i++) {
            if (!vc_cons[i].d)
                    break;
            printk(KERN_INFO "lab3: console[%i/%i] #%i, tty %lx\n", i,
                   MAX_NR_CONSOLES, vc_cons[i].d->vc_num,
                   (unsigned long)vc_cons[i].d->port.tty);
    }
    printk(KERN_INFO "lab3: finished scanning consoles\n");
    lab3_tty_driver = vc_cons[fg_console].d->port.tty->driver;


    lab3_kobj = kobject_create_and_add("lab3_module", kernel_kobj);
    if (!lab3_kobj) {
        return -ENOMEM;
    }

    int error = sysfs_create_file(lab3_kobj, &switcher_attr.attr);
    if (error) {
        pr_info("lab3: failed to create the switcher file in /sys/kernel/lab3_module\n");
    }

    return error;
}

static void __exit lab3_exit(void) {
    pr_info("lab3: module uninitialized\n");
    kobject_put(lab3_kobj);
}

MODULE_DESCRIPTION("Churikov Danil eltex summer 2024 module 5 lab 3");
MODULE_AUTHOR("Danil Churikov");
MODULE_LICENSE("GPL");

module_init(lab3_init);
module_exit(lab3_exit);