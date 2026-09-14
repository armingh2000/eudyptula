# Task 1

## Challenge
Write a Linux kernel module, and stand-alone Makefile, that when loaded prints to the kernel debug log level, "Hello World!"  Be sure to make the module be able to be unloaded as well.

The Makefile should build the kernel module against the source for the currently running kernel, or, use an environment variable to specify what kernel tree to build it against.

Please show proof of this module being built, and running, in your kernel.  What this proof is is up to you, I'm sure you can come up with something.  Also be sure to send the kernel module you wrote, along with
the Makefile you created to build the module.

Remember to use your ID assigned to you in the Subject: line when responding to this task, so that I can figure out who to attribute it to.  You can just respond to the task with the answers and all should be fine.

If you forgot, your id is "X".  But of course you have not
forgotten that yet, you are better than that.

## Solution

In this first task, we write a basic out-of-tree "Hello World" kernel module, build it against the headers of our currently running kernel using Kbuild, and verify that it loads and unloads cleanly.

### Kernel Space vs. User Space

When writing standard C programs in user space, execution starts at `main()`. User-space programs execute in unprivileged CPU mode (**Ring 3**), where memory is virtualized and guarded by hardware. If a program attempts an invalid memory access, the **MMU (Memory Management Unit)** catches it, and the kernel terminates the offending process with a `Segmentation fault` while the rest of the system keeps running uninterrupted. User-space programs also link against runtime libraries (such as `glibc`) to access common functions like `printf()`.

A kernel module works in a fundamentally different way:

* **Supervisor Mode (Ring 0):** Kernel code runs at the highest privilege level with unrestricted access to memory and CPU instructions. There is no memory safety boundary: a bug or null pointer dereference here crashes the whole system with a Kernel Panic.
* **No C Standard Library:** Standard headers like `<stdio.h>` are not available in kernel space. Instead, the kernel provides its own internal functions, such as `printk()`.
* **Event-Driven Execution:** Rather than running sequentially from start to finish like a typical executable, a kernel module registers callback handlers for events (initialization, device requests, interrupts, removal) and remains in kernel memory until explicitly removed.

### A Deeper Look: CPU Protection Rings

Modern CPU architectures (specifically x86) enforce security and isolation using hardware-level privilege levels known as **Protection Rings**. These rings form a hierarchy of privilege, where inner rings have higher authority than outer rings:

```text
[ Ring 3: User Space Applications ]  <-- Lowest Privilege
[ Ring 2: Device Drivers (Unused) ]
[ Ring 1: OS Services (Unused) ]
[ Ring 0: Kernel Space ]  <-- Highest Privilege

```

* **Ring 0 (Supervisor Mode):** The CPU has unrestricted access to the entire instruction set and physical hardware. It can modify control registers (such as `CR0` and `CR3` on x86), disable/enable hardware interrupts (`cli` / `sti`), execute halting instructions (`hlt`), and manipulate raw memory pages. This is where the core Linux kernel, drivers, and loadable kernel modules live.
* **Ring 1 & Ring 2:** Historically designed by CPU architects to hold OS services (like filesystems) and device drivers with intermediate privileges. However, **Linux does not use Ring 1 or Ring 2 at all**. This design choice was made primarily for architectural portability: many other CPU architectures (such as ARM, MIPS, or RISC-V) historically supported only two execution modes (Supervisor and User). Limiting Linux to Ring 0 and Ring 3 ensured the kernel could run uniformly across different processor designs.
* **Ring 3 (User Mode):** The lowest privilege ring. Code running here (browsers, shells, user utilities) cannot execute privileged CPU instructions, cannot directly access I/O ports or hardware registers, and cannot touch memory outside its own allocated address space. Whenever a Ring 3 program needs to perform a privileged task (such as reading a file from disk or sending a network packet), it must issue a **system call** (via instructions like `syscall` or `sysenter`) to request that Ring 0 perform the operation on its behalf.

### The MMU: How Memory Isolation Actually Works

You might wonder: *if a user-space program runs on the exact same physical RAM chips as the kernel, what physically prevents a rogue C pointer from overwriting kernel memory?*

That job belongs to the **MMU (Memory Management Unit)**—a dedicated hardware component embedded directly inside the CPU.

```text
[ Process Virtual Address ] 
           │
           ▼
     ┌───────────┐
     │    MMU    │  <── Walks Page Tables (pointed to by CPU register CR3)
     └─────┬─────┘
           │
     ┌─────┴──────────────────────┐
     │ Valid & Permitted?         │
    YES                          NO
     │                            │
     ▼                            ▼
[ Physical RAM Address ]     [ Hardware Interrupt: PAGE FAULT (#PF) ]
                                  │
                                  ▼
                             [ Kernel do_page_fault() ]
                                  │
                                  ▼
                             Kill process: SIGSEGV (Segmentation Fault)

```

1. **Virtual Addressing:** User-space programs never interact with physical RAM addresses. Every process is given its own isolated, contiguous **Virtual Address Space** (e.g., from `0x000000000000` up to `0x7fffffffffff` on 64-bit Linux). Two completely different processes can access the same virtual address (like `0x400000`) and physically be reading two entirely separate locations in physical RAM.
2. **Page Tables & Translation:** The kernel organizes physical RAM into fixed-size chunks called **Pages** (typically 4 KB each) and constructs hierarchical lookup maps called **Page Tables** in RAM. When a process is scheduled on a CPU core, the kernel loads the physical address of that process's root page table into a special CPU control register (`CR3` on x86). Whenever an instruction reads or writes to memory, the MMU intercepts the virtual address, walks the page table, and translates it into the corresponding physical address.
3. **Hardware Permission Bits:** Every entry in a page table has permission bits attached to it:
* **R/W Bit:** Marks the page as read-only or read/write.
* **U/S Bit (User/Supervisor):** Marks whether Ring 3 code is allowed to access this page, or if it is strictly reserved for Ring 0.
* **NX Bit (No-Execute):** Prevents executing code from data memory (stack/heap protection).


4. **The Page Fault:** If a Ring 3 process tries to write to a read-only page, access an address reserved for Supervisor mode (U/S bit set to 0), or dereference an unmapped address (like a `NULL` pointer `0x0`), the MMU blocks the memory bus and fires a hardware CPU exception: **Page Fault (Exception 14)**.
5. **Handling the Fault:** The CPU instantly drops what it is doing and jumps into the kernel's page fault handler (`do_page_fault()` in Ring 0). If the access was illegal, the kernel delivers a `SIGSEGV` signal to the process, terminating it immediately with **`Segmentation fault (core dumped)`**.

Because kernel modules run directly inside **Ring 0**, they bypass these user-mode MMU guardrails. If a module dereferences an invalid pointer or touches unmapped kernel memory, there is no supervisor above it to terminate the process—resulting in a catastrophic **Kernel Panic** or system freeze.

### 1. The Kernel Module (`helloworld.c`)

Here is our module source code:

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

**Under the Hood:**

* `#include "linux/module.h"`: Provides the core headers, macros, and types required for creating loadable kernel modules.
* `__init`: An optimization hint for the kernel loader. It marks the initialization function so that once the module finishes initializing, the kernel can free that section of memory.
* `__exit`: Tells the kernel that this function is only needed when the code is built as a dynamically removable module. If the code were compiled statically into the kernel image directly, this code would be omitted entirely.
* `printk(KERN_INFO "Hello World!\n")`: The kernel's logging mechanism. `KERN_INFO` is a log level prefix defining the priority of the message in the system ring buffer.
* `mod_exit(void)`: Even if empty, providing an exit function via `module_exit()` is critical. Without an exit function registered, the kernel assumes the module cannot be safely removed, and any call to `rmmod` will be rejected.
* `MODULE_LICENSE("GPL")`: Necessary to declare the license of the module. Without this, the kernel flags itself as "tainted" and restricts access to non-exported GPL kernel symbols.

### 2. The Makefile

Because kernel modules rely on internal kernel data structures and macros, they cannot be compiled using standard `gcc helloworld.c`. We must compile them through the kernel's internal build engine (**Kbuild**):

```makefile
obj-m := helloworld.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

```

**How It Works:**

* `obj-m := helloworld.o`: Informs Kbuild that an object file named `helloworld.o` must be compiled as a loadable module (`.ko`).
* `-C /lib/modules/$(shell uname -r)/build`: Changes directory into the kernel build directory corresponding to our running kernel version (`uname -r`), where the kernel's top-level Makefile resides.
* `M=$(PWD)`: Tells Kbuild to jump back to our current working directory to build the module target defined in `obj-m`.

*(Note: Ensure that the indentation before commands in the Makefile uses actual **Tabs**, not spaces.)*

---

### 3. Build, Run, and Test

**Prerequisites**

To compile against the running kernel on Fedora, make sure the matching kernel headers and development packages are installed:

```bash
sudo dnf install kernel-devel-$(uname -r) kernel-headers

```

**Build the Module**

Run `make` to compile the kernel object:

```bash
make

```


This generates `helloworld.ko`, our compiled kernel module.

**Insert and Verify**

Clear the kernel ring buffer first to isolate our test, then insert the module using `insmod`:

```bash
sudo dmesg -C
sudo insmod helloworld.ko

```

Check the kernel message buffer with `dmesg`:

```bash
dmesg

```

Output:

```text
[ 102.435210] Hello World!

```

We can also confirm that the module is loaded and active in the kernel using `lsmod`:

```bash
lsmod | grep helloworld

```

Output:

```text
helloworld             16384  0

```

**Unload the Module**

Remove the module using `rmmod`:

```bash
sudo rmmod helloworld

```

We can verify that the module has been cleanly removed by querying `lsmod` again:

```bash
lsmod | grep helloworld

```

*(No output returned, confirming clean removal).*

