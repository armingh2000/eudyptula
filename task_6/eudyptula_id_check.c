
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/string.h>

#define ID "7c1caf2f50d1"

static ssize_t misc_read(struct file *fp, char __user *buf,
                    size_t len, loff_t *off)
{
	return simple_read_from_buffer(buf, len, off, ID, strlen(ID));	
}

static ssize_t misc_write(struct file *fp, const char __user *buf,
                    size_t len, loff_t *off)
{
	char data[sizeof(ID)];
	ssize_t ret;

	memset(data, 0, sizeof(data));

	ret = simple_write_to_buffer(data, sizeof(data), off, buf, len);

	if(ret < 0)
		return ret;

	if(!memcmp(ID, data, sizeof(ID))){
		pr_info("correct id value\n");
		return len;
	} else {
		pr_info("invalid id value\n");
		return -EINVAL;
	}

}

static const struct file_operations fops = {
    .write          = misc_write,
    .read           = misc_read,
};


static struct miscdevice md = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "eudyptula",
  .fops = &fops,
};

static int __init mod_init(void) {
	int ret;

	ret = misc_register(&md);

	if(ret){
		pr_err("misc_register failed\n");
		return ret;
	}

	pr_info("eudyptula misc device registered\n");

	return 0;
}

static void __exit mod_exit(void) {
	misc_deregister(&md);
	pr_info("eudyptula misc device unregistered\n");
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Eudyptula challenge 6");
MODULE_AUTHOR("Your Name");
