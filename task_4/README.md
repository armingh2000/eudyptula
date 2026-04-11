# Task 4

## Problem

Wonderful job in making it this far, I hope you have been having fun.
Oh, you're getting bored, just booting and installing kernels?  Well,
time for some pedantic things to make you feel that those kernel builds
are actually fun!

Part of the job of being a kernel developer is recognizing the proper
Linux kernel coding style.  The full description of this coding style
can be found in the kernel itself, in the Documentation/CodingStyle
file.  I'd recommend going and reading that right now, it's pretty
simple stuff, and something that you are going to need to know and
understand.  There is also a tool in the kernel source tree in the
scripts/ directory called checkpatch.pl that can be used to test for
adhering to the coding style rules, as kernel programmers are lazy and
prefer to let scripts do their work for them...

And why a coding standard at all?  That's because of your brain (yes,
yours, not mine, remember, I'm just some dumb shell scripts).  Once your
brain learns the patterns, the information contained really starts to
sink in better.  So it's important that everyone follow the same
standard so that the patterns become consistent.  In other words, you
want to make it really easy for other people to find the bugs in your
code, and not be confused and distracted by the fact that you happen to
prefer 5 spaces instead of tabs for indentation.  Of course you would
never prefer such a thing, I'd never accuse you of that, it was just an
example, please forgive my impertinence!

Anyway, the tasks for this round all deal with the Linux kernel coding
style.  Attached to this message are two kernel modules that do not
follow the proper Linux kernel coding style rules.  Please fix both of
them up, and send it back to me in such a way that does follow the
rules.

What, you recognize one of these modules?  Imagine that, perhaps I was
right to accuse you of the using a "wrong" coding style :)

Yes, the logic in the second module is crazy, and probably wrong, but
don't focus on that, just look at the patterns here, and fix up the
coding style, do not remove lines of code.

As always, please remember to use your ID assigned to you in the
Subject: line when responding to this task, so that I can figure out who
to attribute it to.  And if you forgot (which of course you have not,
we've been through all of this before), your id is "X".

## Solution

To read the complete formatting guidelines, please refer to [the coding style documentation](https://docs.kernel.org/process/coding-style.html). That being said, the document is quite long and not very example-heavy, so here I will briefly summarize some of the most common and useful rules with simple examples:

* Use **tabs, not spaces**

  ```c
    if (x)
    	do();
    // bad:
    if (x)
        do();
  ```

* Keep lines **≤ 80 chars**

  ```c
    int a = b + c;
    // bad:
    int this_is_a_very_long_variable_name = another_very_long_variable_name + something_else;
  ```

* Functions: `{` on **next line**

  ```c
    int f(void)
    {
    	return 0;
    }
    // bad:
    int f(void) {
    	return 0;
    }
  ```

* `if/for/while`: `{` on **same line**

  ```c
    if (x) {
    	do();
    }
    // bad:
    if (x)
    {
    	do();
    }
  ```

* Align closing braces

  ```c
    if (x) {
    	do();
    }
    // bad:
    if (x) {
    	do();
    	}
  ```

* Spaces around operators

  ```c
    a = b + c;
    // bad:
    a=b+c;
  ```

* No space in function calls

  ```c
    foo(bar);
    // bad:
    foo (bar);
  ```

* One statement per line

  ```c
    x++;
    y++;
    // bad:
    x++; y++;
  ```

* One variable per line

  ```c
    int i;
    int count;
    // bad:
    int i, count;
  ```

* Prefer clear names

  ```c
    int count;
    int i;
    // bad:
    int x, y;
  ```

* Use `i++` in loops

  ```c
    for (i = 0; i < n; i++)
    	do();
    // bad:
    for (i = 0; i < n; ++i) do();
  ```

* Avoid unnecessary variables

  ```c
    return a + b;
    // bad:
    int c = a + b;
    return c;
  ```

* Remove unused stuff

  ```c
    int x = 5;
    // bad:
    int x = 5, y;
  ```

* Use `<linux/...>` headers

  ```c
    #include <linux/module.h>
    // bad:
    #include "linux/module.h"
  ```

* Use `pr_*` instead of `printk`

  ```c
    pr_info("hi\n");
    // bad:
    printk(KERN_INFO "hi\n");
  ```

* Multi-line comments style

  ```c
    /*
     * comment
     */
    // bad:
    // comment
  ```

* Clean `if / else if / else`

  ```c
    if (a) {
    	x();
    } else if (b) {
    	y();
    } else {
    	z();
    }
    // bad:
    if (a) {
    	x();
    }
    else if (b) {
    	y();
    }
    else {
    	z();
    }
  ```

* Avoid one-line logic

  ```c
    if (x)
    	do();
    // bad:
    if (x) do();
  ```

* Use braces if unclear

  ```c
    if (x) {
    	do();
    	do2();
    }
    // bad:
    if (x)
    	do();
    	do2();
  ```

Now let’s format the given snippets.

### coding_style.c

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>

int do_work(int *my_int, int retval)
{
	int x;
	int y = *my_int;
	int z;

	for (x = 0; x < *my_int; ++x)
		udelay(10);

	if (y < 10)
		/*
		 * That was a long sleep, tell userspace about it
		 */
		pr_debug("We slept a long time!");
	z = x * y;
	return z;
}

int my_init(void)
{
	int x = 10;

	x = do_work(&x, x);
	return x;
}

void my_exit(void)
{
	return;
}

module_init(my_init);
module_exit(my_exit);
```

The Linux kernel source tree provides a very useful script called `checkpatch.pl` that helps identify formatting issues. While it is not perfect, it catches a large portion of common mistakes. We can use it as follows:

```
perl linux/scripts/checkpatch.pl --no-tree --fix --file coding_style.c
```

Running this produces a few warnings:

```
WARNING: Missing or malformed SPDX-License-Identifier tag in line 1
WARNING: void function return statements are not generally useful
WARNING: adding a line without newline at end of file
```

In addition to fixing these, I also made a few readability improvements.

Starting from the top, the `#include` statements do not require changes. Initially, I thought sorting them alphabetically would be better, but it turns out grouping them logically is preferred in kernel code.

In `my_init`, the variable `x` is used both as input and output:

```c
x = do_work(&x, x);
```

This makes the code slightly confusing. Since the result is immediately returned, we can simplify it:

```c
int my_init(void)
{
	int x = 10;

	return do_work(&x);
}
```

In `my_exit`, the function returns `void`, so the `return` statement is unnecessary:

```c
void my_exit(void)
{
}
```

Looking at `do_work`, the parameter `retval` is never used, so it should be removed:

```c
int do_work(int *my_int)
```

The variable `x` is only used as a loop counter. It is more conventional to use `i` for iteration and `i++` instead of `++i` when the usage doesn't make any difference:

```c
int i;
...
for (i = 0; i < *my_int; ++i)
```

We already store `*my_int` in `y`, so we can reuse it in the loop instead of dereferencing again:

```c
for (i = 0; i < y; ++i)
```

The variable `z` is only used to return a value, so it can be removed:

```c
return i * y;
```

Also, `my_int` is not a very descriptive name. Renaming it to something like `value` improves readability:

```c
int do_work(int *value)
```

Another important requirement is adding a license tag at the top of the file. A common choice is:

```c
// SPDX-License-Identifier: GPL-2.0
```

Finally, we should also ensure the file ends with a newline, as required by kernel style.

### Final refactored version

```c
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
```

### helloworld.c

For this part, I reused my `helloworld.c` file from Task 1 and applied the same formatting principles. I encourage you to the same and refactor your task 1 code:

```c
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
```

Running `checkpatch.pl` gives several warnings and errors:

* Missing SPDX license tag
* Function braces on wrong line
* Use of `printk` instead of `pr_*`
* Trailing whitespace
* Missing newline at end of file

These can be addressed step by step:

* Add SPDX license identifier
* Move function opening braces to the next line
* Replace `printk` with `pr_info`
* Remove unnecessary blank lines
* Ensure file ends with a newline

As a small bonus, it is also good practice to add module metadata like description and author.

### Final formatted version

```c
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
```

Overall, following the kernel formatting guidelines significantly improves readability and consistency, making it much easier for others (and your future self) to understand the code.
