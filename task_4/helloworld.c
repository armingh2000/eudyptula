// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>

static int __init mod_init(void) 
{
	pr_info("Hello World!\n");
	return 0;
}

static void __exit mod_exit(void) 
{
}


module_init(mod_init);
module_exit(mod_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple hello world module");
MODULE_AUTHOR("Your Name");
