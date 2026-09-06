
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/jiffies.h>

static ssize_t jiffies_read(struct file *fp, char __user *buf,
                    size_t len, loff_t *off)
{
    char jiffies_decoded[32];

    snprintf(jiffies_decoded, sizeof(jiffies_decoded), "%lu\n", jiffies);

    return simple_read_from_buffer(buf, len, off, jiffies_decoded, strlen(jiffies_decoded));
}

static const struct file_operations fops_jiffies = {
    .read   = jiffies_read,
};


