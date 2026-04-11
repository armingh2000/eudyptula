// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

int do_work(int *value)
{
	int i;
	int val = *value;

	for (i = 0; i < val; i++)
		udelay(10);

	if (val < 10) {
		/* 
		* That was a long sleep, tell userspace about it 
		*/
		pr_debug("We slept a long time!");
	}

	return i * val;
}

int my_init(void)
{
	int x = 10;

	return do_work(&x);
}

void my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
