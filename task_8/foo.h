
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <asm/page.h>


char foo_data[PAGE_SIZE + 1] = {0};
DEFINE_MUTEX(foo_lock);


static ssize_t foo_read(struct file *fp, char __user *buf,
                    size_t len, loff_t *off)
{
    ssize_t ret;

    mutex_lock(&foo_lock);
    ret = simple_read_from_buffer(buf, len, off, foo_data, PAGE_SIZE);
    mutex_unlock(&foo_lock);

    return ret;
}

static ssize_t foo_write(struct file *fp, const char __user *buf,
                    size_t len, loff_t *off)
{
    ssize_t ret;

    mutex_lock(&foo_lock);
    memset(foo_data, 0, sizeof(foo_data));
    ret = simple_write_to_buffer(foo_data, PAGE_SIZE, off, buf, len);
    mutex_unlock(&foo_lock);

    return ret;
}

static const struct file_operations fops_foo = {
    .read   = foo_read,
    .write  = foo_write,
};


