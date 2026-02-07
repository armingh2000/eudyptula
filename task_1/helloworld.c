
#include "linux/module.h"

static int __init mod_init(void) {
	printk(KERN_INFO "Hello World!\n");

	return 0;
}

static void __exit mod_exit(void) {
	
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
