# Task 2

## Challenge

Now that you have written your first kernel module, it's time to take
off the training wheels and move on to building a custom kernel.  No
more distro kernels for you, for this task you must run your own kernel.
And use git!  Exciting isn't it!  No, oh, ok...

The tasks for this round is:

* download Linus's latest git tree from git.kernel.org (you have to
figure out which one is his, it's not that hard, just remember what
his last name is and you should be fine.)
* build it, install it, and boot it.  You can use whatever kernel
configuration options you wish to use, but you must enable
CONFIG_LOCALVERSION_AUTO=y.
* show proof of booting this kernel.  Bonus points for you if you do
it on a "real" machine, and not a virtual machine (virtual machines
are acceptable, but come on, real kernel developers don't mess
around with virtual machines, they are too slow.  Oh yeah, we aren't
real kernel developers just yet.  Well, I'm not anyway, I'm just a
script...)  Again, proof of running this kernel is up to you, I'm
sure you can do well.

Hint, you should look into the 'make localmodconfig' option, and base
your kernel configuration on a working distro kernel configuration.
Don't sit there and answer all 1625 different kernel configuration
options by hand, even I, a foolish script, know better than to do that!

After doing this, don't throw away that kernel and git tree and
configuration file.  You'll be using it for later tasks, a working
kernel configuration file is a precious thing, all kernel developers
have one they have grown and tended to over the years.  This is the
start of a long journey with yours, don't discard it like was a broken
umbrella, it deserves better than that.

Remember to use your ID assigned to you in the Subject: line when
responding to this task, so that I can figure out who to attribute it
to.

If you forgot, your id is "X".  But why do I repeat myself?
Of course you know your id, you made it through the first task just fine
with it.

---

## Solution

### Clone Latest Kernel

Visit [git.kernel.org](https://git.kernel.org) and search for Torvalds (Linux kernel source tree). Once you open the tree, you can clone it using the links at the bottom of the page in the **Clone** section.

### Installing Prerequisites

*Fedora*

For building the Linux kernel, certain packages and headers need to be installed:

```bash
sudo dnf install kernel-devel-$(uname -r) kernel-headers

```

### Preparing the Config File

The Linux kernel has plenty of build options to enable or disable specific features. When building it, we need a config file to specify which options to include.

We can build a configuration file tailored to our system by running:

```bash
make olddefconfig

```

*Reference: [Configuration](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html#configuration:~:text=Create,-the)*

Now, the challenge requires us to enable `CONFIG_LOCALVERSION_AUTO`. We can achieve this by using the kernel's built-in script:

```bash
./scripts/config -e CONFIG_LOCALVERSION_AUTO

```

*Reference: [Config Script](https://docs.kernel.org/admin-guide/verify-bugs-and-bisect-regressions.html#introworkingcheck-bissbs:~:text=Ensure,-all)*

Once done, we can quickly check whether this option is now enabled in our config file:

```bash
grep CONFIG_LOCALVERSION_AUTO .config
# OR
./scripts/config --state CONFIG_LOCALVERSION_AUTO

```

**Bonus Point (Optional)**

Once we build our kernel, it is nice to be able to verify whether the `CONFIG_LOCALVERSION_AUTO` flag is actually enabled on the running kernel. To achieve this, we must enable two extra options so that our compiled kernel stores its own configuration where we can view it:

```bash
./scripts/config -e CONFIG_IKCONFIG -e CONFIG_IKCONFIG_PROC

```

> `CONFIG_IKCONFIG=y` (This embeds the config).
> `CONFIG_IKCONFIG_PROC=y` (This exposes it to /proc).

### Build

Once we have the config file ready, we can simply build our configured kernel using:

```bash
make -j $(nproc --all)

```

*Reference: [Build](https://docs.kernel.org/admin-guide/quickly-build-trimmed-linux.html#configuration:~:text=Now,-build)*

> This can take a while depending on your system specs.

### Boot

Now that we have the binary executable file, we need to boot it and test if it works. We can either use our own system to run the kernel, or we can use a virtual machine.

Personally, I preferred to load it in a virtual machine to keep my host machine isolated from any unintentional changes. Hence, I will only explain how I booted the kernel in QEMU.

**QEMU Setup**

As noted in the [QEMU documentation](https://www.qemu.org/docs/master/system/linuxboot.html), a direct Linux boot typically uses a command like this:

```bash
qemu-system-x86_64 -kernel bzImage -drive file=rootdisk.img,format=raw -append "root=/dev/sda console=ttyS0" -nographic

```

As we can see, the drive is specified as a file `rootdisk.img,format=raw`. This requires us to create a virtual hard drive, format it with a filesystem, and install an OS on it to successfully run the kernel from the disk.

There is a shortcut to bypass this. Instead of using `-drive` and creating a persistent file with a filesystem, we can use `-initrd` to give the kernel a temporary root filesystem that lives entirely in RAM. We will explore only this method in the following section. Ultimately, our final QEMU command will look like this:

```bash
qemu-system-x86_64 -kernel arch/x86/boot/bzImage -initrd initramfs.cpio.gz -append "console=ttyS0" -nographic

```

**A Little View Under the Hood**

Let's dive a bit deeper into what happens when we boot a system to understand what we need to prepare for a successful boot.

When QEMU starts a session, it loads the kernel directly into RAM. At this point, the kernel is blind and has no user-space programs to run. Once the kernel is loaded, the bootloader (QEMU in this case) passes it essential Boot Parameters. One of these parameters tells the kernel where to find the root filesystem, which contains the programs necessary to set up the system properly.

A root filesystem, however, is not just a simple empty space. It needs to follow a specific hierarchy (`/bin`, `/sbin`, `/etc`, `/proc`, `/sys`).

Moreover, if we were booting from a real or virtual hard drive, we would have to format it. A filesystem (FS) is the organizational software layer applied on top of raw disk space. It is a specific data structure (like `ext4`, `NTFS`, or `btrfs`) that acts as a giant index and map. It translates human-readable concepts (like `/usr/bin/python`) into the physical block addresses the hardware understands. It tracks:

* **Metadata:** Who owns the file? When was it created? Who is allowed to read it?
* **Hierarchy:** Which files belong inside which directories?
* **Free Space Management:** Which blocks are currently empty and safe to write new data to?

Linux uses a VFS (Virtual File System) to translate kernel system calls such as `open()`, `read()`, and `write()` across all these different filesystems.

Finally, the filesystem must contain an `init` executable. This file contains the very first instructions for what the kernel needs to do once it is up and running. Without an `init` process, the system stalls because the kernel doesn't know what to do next. The `init` program mounts necessary virtual drives, sets up the environment, and starts the initial shell terminal for us to interact with.

In summary, to bypass the need for an `ext4` formatted virtual hard drive, we are going to create a lightweight, temporary root filesystem in RAM (an `initramfs`) that contains our directory structure and our `init` instructions.

**Create the INITRD Image**

**1. Create a workspace**

```bash
mkdir -p ~/initdisk/initramfs
cd ~/initdisk

```

**2. Get BusyBox**

BusyBox is an engineering miracle that acts as a complete "Swiss Army Knife" for embedded Linux.

When running standard binaries (like `ls` or `bash`), they usually interact dynamically with shared C libraries (like `glibc`). These libraries contain the instructions for commonly used functions (`printf`, `open`, etc.) and often include many highly optimized versions of the same code for different CPU architectures. Dynamically linked binaries need these massive libraries to be present on the disk to run.

BusyBox helps us avoid including these huge libraries where we don't have enough space (like inside a RAM disk or a router). Instead of relying on external libraries at run-time, BusyBox is **statically linked**.

To save even more space, it is typically compiled against alternative, lightweight C libraries (like `musl`) that prioritize tiny, pure C code over large, architecture-specific assembly optimizations. The developers bake all the necessary functions directly into the BusyBox binary itself, making it entirely self-contained.

To get the BusyBox binary, we can either download a pre-compiled version or build our own from the source tree. For the purpose of this challenge, I chose a pre-compiled, statically linked binary.

```bash
wget https://busybox.net/downloads/binaries/1.31.0-defconfig-multiarch-musl/busybox-x86_64 -O busybox
chmod +x busybox

```

**3. Build Directory Structure**

As discussed previously, the image requires a standard Linux directory hierarchy to be present.

```bash
cd initramfs
mkdir -p bin sbin etc proc sys usr/bin usr/sbin

```

**4. Include BusyBox in the system**

```bash
cp ../busybox bin/

```

**5. Create the `init` Script**

We need to create a file named `init` at the root of our folder and put the commands below in it. Notice how it tells BusyBox to install symlinks for all common commands (like `ls`, `cat`, etc.) so they all point back to the BusyBox binary:

```bash
#!/bin/busybox sh

# Install all busybox symlinks (so 'ls' points to 'busybox', etc.)
/bin/busybox --install -s /bin

# Mount necessary virtual filesystems
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo "====================================="
echo " Booted successfully! Welcome to sh! "
echo "====================================="

# Hand over control to the shell
exec /bin/sh
```

Once created, we must make the file executable (otherwise the kernel will panic because it can't run it):

```bash
chmod +x init

```

**6. Package the initramfs**

The kernel expects the initramfs to be packaged in the `cpio` format (specifically the `newc` format) and typically compressed with `gzip`.

```bash
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz

```

**Boot**

Finally :), we can jump up one directory and boot into our new system by running:

```bash
cd ..
qemu-system-x86_64 -kernel arch/x86/boot/bzImage -initrd initramfs.cpio.gz -append "console=ttyS0" -nographic

```

### Check Running Config

Since we already set the specific config flags during the build process, we can now view the configuration of our running kernel from inside the QEMU virtual machine by running:

```bash
zcat /proc/config.gz | grep CONFIG_LOCALVERSION_AUTO

```
