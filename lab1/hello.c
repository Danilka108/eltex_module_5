#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("Danilka108License");
/*MODULE_LICENSE("GPL");*/
MODULE_AUTHOR("danilka108");
MODULE_DESCRIPTION("Danil Churikov first linux module");

static __init int hello_init(void) {
  printk(KERN_INFO "hello world\n");
  return 0;
}

static __exit void hello_cleanup(void) {
  printk(KERN_INFO "Cleaning up module\n");
}

module_init(hello_init);
module_exit(hello_cleanup);
