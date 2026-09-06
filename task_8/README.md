# Task 8

# Challenge

We will come back to the linux-next kernel in a later exercise, so don't
go and delete that directory, you'll want it around.  But enough of
building kernels, let's write more code!

This task is much like the 06 task with the misc device, but this time
we are going to focus on another user/kernel interface, debugfs.  It is
rumored that the creator of debugfs said that there is only one rule for
debugfs use, "There are no rules when using debugfs."  So let's take
them up on that offer and see how to use it.

debugfs should be mounted by your distro in /sys/kernel/debug/, if it
isn't, then you can mount it with the line:
        mount -t debugfs none /sys/kernel/debug/

Make sure it is enabled in your kernel, with the CONFIG_DEBUG_FS option,
you will need it for this task.

The task, in specifics is:

  - Take the kernel module you wrote for task 01, and modify it to be
    create a debugfs subdirectory called "eudyptula".  In that
    directory, create 3 virtual files called "id", "jiffies", and "foo".
  - The file "id" operates just like it did for example 06, use the same
    logic there, the file must be readable and writable by any user.
  - The file "jiffies" is to be read only by any user, and when read,
    should return the current value of the jiffies kernel timer.
  - The file "foo" needs to be writable only by root, but readable by
    anyone.  When writing to it, the value must be stored, up to one
    page of data.  When read, which can be done by any user, the value
    must be returned that is stored it it.  Properly handle the fact
    that someone could be reading from the file while someone else is
    writing to it (oh, a locking hint!)
  - When the module is unloaded, all of the debugfs files are cleaned
    up, and any memory allocated is freed.
  - Provide some "proof" this all works.

Again, you are using your id in the code, so you know what it is by now,
no need to repeat it again.

# Solution

As usual, to familiarize with `debugfs`, please refer to the [DebugFS Documentation](https://docs.kernel.org/filesystems/debugfs.html#debugfs) available on the Kernel website. I will, however, briefly explain the purpose of different filesystems below. Then, we will continue with the implementation of the code, which is similar to the character device challenge (task 6).

# FileSystems
In Linux, there is a famous philosophy: "**Everything is a file.**"

Filesystems are essentially systems of files (and directories) that let user space communicate with drivers or the kernel. By system, we are talking about sets of files that are built for specific purposes. All of these filesystems are virtual, meaning they exist entirely in RAM. When we read one of these files (using `cat` for example), the kernel intercepts that request and fetches the data for us based on the logic that is tied to that file. Similarly, when we write to one of these files (using `echo` for example), the kernel passes our text directly to a driver that is tied to that file. In other words, on each action (read/write/execute) on these files, there is defined `C` code logic that handles that action with its parameters or inputs.

## Three main FS: `procfs`, `sysfs`, `debugfs`

As mentioned previously, each of these filesystems satisfies specific needs of developers, and they come with strict boundaries that everyone must follow.

**`procfs` (mounted at `/proc`)**: Contains process information and general system-wide metrics. Originally designed to hold information about running processes. Over time, developers threw general system information in there too (like CPU and memory stats). It has very strict rules on keeping backward compatibility; changes are avoided because they can break user-space tools. This is heavily used in production.

**`sysfs` (mounted at `/sys`)**: Contains the hardware device model and drivers. It was created to represent the computer's actual hardware layout. It is highly organized with a strict rule: *a file should only contain a single value* (like 1 for on, or 0 for off). It also has strict rules on keeping backward compatibility; changes are avoided because they break hardware configuration tools. This is heavily used in production.

**`debugfs` (mounted at `/sys/kernel/debug`)**: A pure debugging playground for kernel developers. This was made only for debugging purposes since the strict rules for `procfs` and `sysfs` were not suitable for quick debugging. For example, if a developer wanted to write a new Wi-Fi driver and dump 50 lines of internal chip register states, they couldn't use `sysfs` due to violating the single-value rule, or `procfs` due to violating API stability. So this sandbox filesystem was created solely for debugging purposes. There are practically no rules on using this filesystem. This is not meant to be used in production and is often disabled entirely on production machines.

Now that we briefly discussed the filesystems and their purpose, let's continue with the main challenge.

# File Structure

Since this challenge combines three different debugfs files, I chose to separate the logic for each file into separate `.h` header files and create a main `.c` file which combines the logic from all the header files:

```
task_8.c
|
 ------- id.h
|
 ------- jiffies.h
|
 ------- foo.h
```

In our main file, i.e. `task_8.c`, we need to create the `eudyptula` subdirectory inside debugfs and have the three files inside this subdirectory:

```c
static int __init mod_init(void) {
	parent = debugfs_create_dir("eudyptula", NULL);
	debugfs_create_file("id", 0666, parent, NULL, &fops_id);
	debugfs_create_file("jiffies", 0444, parent, NULL, &fops_jiffies);
	debugfs_create_file("foo", 0644, parent, NULL, &fops_foo);

	pr_info("registered eudyptula directory in debugfs\n");

	return 0;
}
```

Here, as you can see, `debugfs_create_dir` takes care of the subdirectory creation. We save the result of this function in a global `struct dentry *parent` pointer so we can pass it to `debugfs_create_file`. Another point worth mentioning is the access codes (`0666`, `0444`, `0644`) used here. These access codes correspond to the access restrictions explained in the challenge.

For learning about Linux file permission logic and how it works, I highly recommend this [Red Hat - Linux File Permissions Explained](https://www.redhat.com/en/blog/linux-file-permissions-explained) webpage, as they clearly explain how it works.

# id

Since we already created the `id` character device driver in task 6, this portion of the challenge is already done. All we need to do is to have the logic in our `id.h` file and use it in the `task_8.c` main file.

# jiffies

To understand what `jiffies` is (are?!) please refer to the [Oracle Documentation](https://blogs.oracle.com/linux/jiffies-the-heartbeat-of-the-linux-operating-system).

To output the `jiffies` variable to user-space in the terminal, we need to convert it to a string format and pass it securely. `jiffies` is an unsigned long, meaning on a 64-bit system it can hold the maximum value of **18,446,744,073,709,551,615**, which has 20 digits. Each of these digits would be a character in the output we pass to user-space. At the end of our strings, we have a null character `\0` which determines the end of the string. For this challenge specifically, we also want to add a newline character `\n` to the end of our string before it ends (before `\0`). So, overall we want to output the string below whenever the `read` function of our file is called:

`jiffies (20 chars) + \n (1 char) + \0 (1 char) = 22 chars total`

Hence, our string variable should be at least 22 characters long to accommodate the maximum possible output. However, in the Linux kernel, it is often best practice to align array sizes to powers of 2. In effect, we can create an array of characters with a length of 32:

```c
char jiffies_decoded[32];
```

To read the `jiffies` variable and convert it from raw data bytes to string formatting character by character, we can use a built-in kernel tool called `snprintf` which takes care of this formatting for us:

```c
snprintf(jiffies_decoded, sizeof(jiffies_decoded), "%lu\n", jiffies);
```

*(The `%lu` in the above expression specifies that the `jiffies` variable is a long unsigned integer).*

Once formatted, we use `simple_read_from_buffer()` to safely pass `jiffies_decoded` to user space.

Before explaining the logic behind the final file, let's take a quick look at the locking mechanisms in Linux.

# Locking Mechanisms in Linux

There are multiple locking mechanisms in the Linux kernel, 4 of which are explained below:

* **Spinlocks**: When a thread tries to take a spinlock that is already taken, it enters a tight `while` loop, constantly asking the CPU if the lock is free. This type of lock is used in the **Interrupt Context** where sleeping is physically impossible. If a thread holding this lock goes to sleep, the CPU core will be entirely consumed by the infinite `while` loop, and the system will hard-freeze and panic. **Example:** Your mouse moves, sending an electrical interrupt. The kernel must instantly pause what it's doing, grab a spinlock, update the mouse X/Y coordinates in memory, release the lock, and resume.

* **Mutexes**: When a thread tries to take a mutex that is already taken, it puts itself to sleep and lets the CPU run other tasks. This type of lock is used, for example, when copying data to/from user space (which can trigger page faults and cause a sleep). If a mutex is deadlocked (locked indefinitely), the task that is trying to acquire that lock hangs, but the rest of the OS will keep running just fine. **Example:** A user runs a script to format a USB drive. The USB driver grabs a mutex. If another script tries to format the same drive, it hits the mutex and simply goes to sleep until the first script finishes.

* **Read/Write Semaphores**: This splits access into two lanes. Readers can share the lock. But if a writer wants to change the data, it must wait for all readers to finish, lock everyone out exclusively, make the change, and then let the readers back in. This type of lock is used when the ratio of reads to writes is massive (e.g., 99% reads, 1% writes). If readers never stop coming in, the lock never reaches "zero readers" and the writer is trapped waiting forever (modern Linux has algorithms to prevent this, but it remains a theoretical risk). **Example:** A firewall routing table. Thousands of network packets need to read the rules simultaneously to pass through, but the system administrator only updates the rules once a month.

* **RCU (Read-Copy-Update)**: Readers never lock! If a writer needs to change the data, it behaves like a purely functional data operation: instead of mutating the data in place, it makes a complete copy, updates the copy, and swaps the pointer. It waits for the old readers to finish looking at the old data, and then deletes the old copy. This type of lock is used in highly parallel, performance-critical subsystems where even the overhead of checking a read/write semaphore is considered too slow. If the writer forgets to clean up the old copies, memory fills up. If the logic is slightly off, a reader might act on "stale" (outdated) data. **Example:** Updating the list of loaded kernel modules while the system is under heavy load. The system builds the new list in the background and hot-swaps it instantly.

# foo

Now that we have a basic understanding of locking mechanisms, we can explain the logic of our `foo` file.

Since this is a simple `debugfs` file and it is not highly concurrent or performance-critical, we can opt for a `mutex` lock to restrict simultaneous reads and writes to our shared resource (the data array that will hold what root writes to this file). All we need to do is define our lock and wrap our actual read/write functions inside the lock/unlock functions:

```c
// Define
DEFINE_MUTEX(foo_lock); 

// Wrap
mutex_lock(&foo_lock);
...
mutex_unlock(&foo_lock);
```

Another note worth pointing out is that even though the `root` user can only write `PAGE_SIZE` worth of data to this file, we dedicate `PAGE_SIZE + 1` as our buffer size. `PAGE_SIZE` is a macro provided by `<asm/page.h>` (usually 4096 bytes):

```c
char foo_data[PAGE_SIZE + 1] = {0};
```

When writing to our buffer, we can re-initialize this array with null characters. This is because if the `root` user's new input is shorter than their previous input, the string won't stop at the end of the new input and will read into the remainder of the previous input, causing a data leak:

```c
memset(foo_data, 0, sizeof(foo_data));
```

Finally, to make sure that after the removal of the module, all these files and their parent directory are unloaded too, we use the `debugfs_remove` function to take care of the cleaning for us in our module exit function. Because `debugfs_remove` operates recursively, pointing it at the parent directory cleans up the directory and all three files inside it automatically:

```c
debugfs_remove(parent);
```

## Testing

For testing the files with writes and reads, we can use the commands explained in task 6:

```bash
echo -n "TEXT" | sudo tee /sys/kernel/debug/eudyptula/[id, foo]

cat /sys/kernel/debug/eudyptula/[id, jiffies, foo]

```

When we are testing the validity of the module, we need to read/write as different users: once with our own standard user and once with `root`, based on the challenge requirements.

Sometimes, `debugfs` is restricted to `root` by default and no other user can access it, which halts our testing for standard user reads. To grant access to all users temporarily (resets after reboot), we can use:

```bash
sudo mount -o remount,mode=755 /sys/kernel/debug
```