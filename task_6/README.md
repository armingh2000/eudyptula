# Task 6

# Challenge

Nice job with the module loading macros, those are tricky, but a very
valuable skill to know about, especially when running across them in
real kernel code.

Speaking of real kernel code, let's write some!

The task this time is this:
  - Take the kernel module you wrote for task 01, and modify it to be a
    misc char device driver.  The misc interface is a very simple way to
    be able to create a character device, without having to worry about
    all of the sysfs and character device registration mess.  And what a
    mess it is, so stick to the simple interfaces wherever possible.
  - The misc device should be created with a dynamic minor number, no
    need running off and trying to reserve a real minor number for your
    test module, that would be crazy.
  - The misc device should implement the read and write functions.
  - The misc device node should show up in /dev/eudyptula.
  - When the character device node is read from, your assigned id is
    returned to the caller.
  - When the character device node is written to, the data sent to the
    kernel needs to be checked.  If it matches your assigned id, then
    return a correct write return value.  If the value does not match
    your assigned id, return the "invalid value" error value.
  - The misc device should be registered when your module is loaded, and
    unregistered when it is unloaded.
  - Provide some "proof" this all works properly.

As you will be putting your id into the kernel module, of course you
haven't forgotten it, but just to be safe, it's "7c1caf2f50d1".

# Solution

For a brief introduction to mis[cellaneous] drivers, you can refer to [this Embetronicx webpage](https://embetronicx.com/tutorials/linux/device-drivers/misc-device-driver/). To see what an actual kernel driver looks like, you can also refer to the [hpilo.c driver](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/misc/hpilo.c?h=v7.1), which has examples of read and write functions for misc drivers.

## Misc Driver Structure

The misc driver structure in the kernel is defined with `miscdevice`, and it has a couple of attributes that we care about here:

- minor: the minor number for this driver. Every driver in the kernel has two numbers, major and minor, which need to be set in the driver implementation. For misc drivers, the major number is fixed, so we only need to set the minor number. As the challenge says, we can use `MISC_DYNAMIC_MINOR`, so we don't have to worry about finding a free minor number ourselves.
- name: the name of the misc driver. This specifies the path `/dev/NAME`, through which we can access the `read` and `write` functions of the driver.
- fops: the `file_operations` structure of the kernel, which lets us define different functions for our driver, such as `read` and `write`.

We can set up the mentioned attributes for our misc driver as below:

```c
static struct miscdevice md = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "eudyptula",
  .fops = &fops,
};
```

For the `file_operations` structure, we only need to specify the `read` and `write` functions like below:

```c
static const struct file_operations fops = {
    .write          = misc_write,
    .read           = misc_read,
};
```

Now we need to implement our `misc_read` and `misc_write` functions. Both functions need a way to talk to user-space: one to receive input, and one to send output. If we look at the [open-dice.c driver](https://elixir.bootlin.com/linux/v7.1.2/source/drivers/misc/open-dice.c), we can see its read function is called `open_dice_read`. It uses the `simple_read_from_buffer` function, which is exactly what we need!

So, we will need to use `simple_read_from_buffer` and `simple_write_to_buffer` to communicate with user-space. For more on how these work, you can check [the kernel documentation](https://www.kernel.org/doc/html/v5.0/filesystems/index.html#c.simple_read_from_buffer).

## How to implement the `write` function?

Here, we mainly need to receive the user's input from user-space and compare it with the ID string. For this, we need somewhere in kernel space to store that data. So we declare a character array:

```c
char data[sizeof(ID)];
```

Remember, it's important to give our data a real size, and not just use `char *data;`. Otherwise, we are creating a pointer to memory that doesn't exist yet, no space has actually been set aside for it. This can end up corrupting the kernel's memory, or getting us denied access to memory that isn't ours. Or, if we are lucky, it points to some other memory block that happens to be writable, and things appear to work. But we don't want to write a driver that only works if the stars align, do we???

*Note: Here `sizeof(ID)` equals 13, even though there are only 12 characters in the ID. This is because all strings in C end with a null character.*

We also need to make sure the space in this array is set to 0 first, using:

```c
memset(data, 0, sizeof(data));
```

This sets all the bytes in the array to 0 (or null). Why is this important? Imagine we don't do this, and the array just keeps whatever random values were already there. Now imagine the user sends the full ID, "7c1caf2f50d1". This is only 12 bytes, so the last byte of our array is never touched, and keeps its old random value. If we are lucky, that last byte already happens to be "\0", so the comparison passes, just like it should. But what if that leftover byte is something else, which is actually very likely? Then, even though the user sent the correct ID, the comparison fails, because the last byte of our array doesn't match the "\0" at the end of `ID`.

Finally, once our comparison is completed and we process the correct ID, we can return the length of the input. This is common practice in the kernel: `write` functions usually return the number of bytes they process, `len` in our case.

The rest of the function is simple to understand, so we will stop here.

## A note on the Read function

The same issue we explained above for `write` also applies to `read`. If we pass `sizeof(ID)` as the length, we send back 13 bytes instead of 12, since the last byte is just the null terminator `\0`. Using `strlen(ID)` instead fixes this.

# Run and Test

Once we have our driver code ready, we can compile it and add the module to the kernel using `insmod`. To test it, we create a live `dmesg` session using `sudo dmesg -w`. Then, in another terminal window, we run `sudo cat /dev/eudyptula` to read from our driver. This should print the ID. 

To test the write function, we use `echo -n "7c1caf2f50d1" | sudo tee /dev/eudyptula > /dev/null` to write the correct ID into the driver. Then, if we look at the live `dmesg` session, we see "correct id value" in the log. 

To test the driver with a wrong ID, we simply change the command to `echo -n "oih8g942" | sudo tee /dev/eudyptula > /dev/null`. This time, the write fails with the message "tee: /dev/eudyptula: Invalid argument", and the kernel log shows "invalid id value", which is expected.