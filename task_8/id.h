
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/string.h>

#define ID "7c1caf2f50d1"

static ssize_t id_read(struct file *fp, char __user *buf,
                    size_t len, loff_t *off)
{
	return simple_read_from_buffer(buf, len, off, ID, strlen(ID));	
}

static ssize_t id_write(struct file *fp, const char __user *buf,
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

static const struct file_operations fops_id = {
    .write	= id_write,
    .read 	= id_read,
};
