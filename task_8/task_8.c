
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include "id.h"
#include "jiffies_.h"
#include "foo.h"

static struct dentry *parent;


static int __init mod_init(void) {
	parent = debugfs_create_dir("eudyptula", NULL);
	debugfs_create_file("id", 0666, parent, NULL, &fops_id);
	debugfs_create_file("jiffies", 0444, parent, NULL, &fops_jiffies);
	debugfs_create_file("foo", 0644, parent, NULL, &fops_foo);

	pr_info("registered eudyptula directory in debugfs\n");

	return 0;
}

static void __exit mod_exit(void) {
	debugfs_remove(parent);
	pr_info("eudyptula debufs directory unregistered\n");
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Eudyptula challenge 8");
MODULE_AUTHOR("Your Name");
