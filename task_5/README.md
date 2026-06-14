# Task 5

## Challenge

Yeah, you survived the coding style mess! Now, on to some "real" things, as I know you are getting bored by these so far.

So, simple task this time around:

* take the kernel module you wrote for task 01, and modify it so that when a USB keyboard is plugged in, the module will be automatically loaded by the correct userspace hotplug tools (which are implemented by depmod / kmod / udev / mdev / systemd, depending on what distro you are using.)

Yes, so simple, and yet, it's a bit tricky. As a hint, go read chapter 14 of the book, "Linux Device Drivers, 3rd edition." Don't worry, it's free, and online, no need to go buy anything.

As always, please remember to use your ID assigned to you, yadda yadda yadda... It's "7c1caf2f50d1" of course.

## Solution

*Before reading through this solution, as indicated by the challenge, please refer to chapter 14 of the [Linux Device Drivers, 3rd edition](https://lwn.net/Kernel/LDD3/).*

Whenever a device is added to or removed from a system, the kernel dispatches a hotplug event (known as a `uevent`) regarding that device. When this happens, user-space device managers like `udev` catch the event, iterate over the "rules" defined in the `/etc/udev/rules.d/` directory, and take action based on the logic defined by each matched rule.

Hence, for this challenge, what we need to do is install our module into the system's module tree and write our own hotplug rule. This way, whenever a USB device is added to the system, if our module is not loaded already, the system dynamically loads it into the kernel.

There is a fundamental difference in how we handle modules: loading them manually directly into **RAM** (using `insmod`), OR installing them onto the **disk** and "letting the kernel know" where they are so user-space tools can load them dynamically by name on demand.

For this challenge, there is a downside to the manual RAM approach. Whenever we load a module directly into memory with `insmod`, the kernel executes its `mod_init` function immediately and the module begins running. If we take this approach and load our module in advance, we will see the "Hello World" message printed to the kernel log right then. Now, if we add a USB drive to the system, our hotplug tools will try to load the module again. However, since the module is already loaded, there is nothing left to do and the load command simply exits with a success code. Hence, we won't see anything printed to the kernel log *after* inserting the USB device, which is not what we want.

*Note: A module can only be loaded into the kernel once before being removed. Any further requests to load the module will result in no action from the kernel, as it simply returns a success code acknowledging the module is already present.*

Because we want to see the "Hello World" print *only* after we insert the USB, we need to follow the second approach: telling the system where the module resides on disk so it can be automatically loaded at the exact right time.

To do this, we need to understand what a module dependency map is and how it operates.

**Module Dependency Map**: When a tool like `modprobe` wants to load a module by its name, it needs to map that name to the actual kernel object binary file. It does this using the Dependency Map, which is generated based on the modules available in `/lib/modules/$(uname -r)/`. Once this index is created, the system knows exactly where to find and load a module whenever hardware probing requires it.

Now that we know how this works, we can compile our module, copy it to the modules directory, update the Dependency Map, create our hotplug rule, tell `udev` to register our rule, and finally test it by inserting a USB device. Simple, eh? :) Let's get started!

First, we need to use the C code and the Makefile from task_1 to compile our module and create a kernel object binary:

```bash
make -C /lib/modules/$(uname -r)/build M=$PWD
```

Now that we have our kernel object binary (the `.ko` file), we can copy it to the modules directory on the disk:

```bash
sudo mkdir -p /lib/modules/$(uname -r)/extra/
sudo cp helloworld.ko /lib/modules/$(uname -r)/extra/
```

*Note: Here, I am copying the module into a separate folder named `extra` to keep custom changes cleanly isolated.*

Next, we need to update the Dependency Map so the system knows a module named `helloworld` actually exists:

```bash
sudo depmod -a
```

Up next, we create our hotplug rule file at `/etc/udev/rules.d/usb.rules` and put the following logic inside:

```udev
ACTION=="add", SUBSYSTEM=="usb", RUN+="/usr/bin/modprobe helloworld" 
```

*Note: Initially, I just used `RUN+="modprobe helloworld"`, but after inserting a USB device, I got the error "Failed to find and pin callout binary '/usr/lib/udev/modprobe': No such file or directory" in my `journalctl` logs. Apparently, whenever a command is used without an absolute path, `udev` assumes you are trying to execute a built-in internal helper tool and automatically prepends `/usr/lib/udev/` to the command, breaking the path. To solve this, you need to use the absolute path of `modprobe` (which you can find by running `which modprobe`) directly in your rule logic.*

Now that the rule is in the designated directory, we need to force `udev` to register it:

```bash
sudo udevadm control --reload-rules
```

We now have everything in place and ready for a test. Make sure your kernel log is clear by running `sudo dmesg -C`. We can now insert a physical USB device, view the kernel log by running `dmesg`, and we should see our "Hello World" string pop up!

*Note: As mentioned earlier, each module can only be loaded into the kernel once before being removed. Hence, if we continuously add more USB devices at this point, we won't see any further "Hello World" messages in the kernel log, as our module won't execute its initialization code again.*

**Bonus tip:** Since physically removing and reinserting a USB drive every time is not a practical way to debug a hotplug script, we can use a trick to trigger our rule synthetically. The `udevadm` utility has a tool that forces a replay of all rules associated with a specific device type (like USB):

```bash
sudo udevadm trigger --action=add --subsystem-match=usb
```

This makes testing incredibly easy. Just keep in mind that before running the trigger, we still need to unload our module from memory so it can be loaded fresh. In conclusion, to retry our module without physically touching a USB device, we can simply run the following sequence:

```bash
sudo rmmod helloworld

sudo udevadm trigger --action=add --subsystem-match=usb

dmesg
```

### Clean Up

To clean up the changes made to the system, you can run the following:

```bash
# 1. Force the module out of live system memory
sudo rmmod helloworld

# 2. Delete your custom rule file
sudo rm /etc/udev/rules.d/usb.rules

# 3. Delete your compiled module from the kernel tree
sudo rm -rf /lib/modules/$(uname -r)/extra/helloworld.ko

# 4. Tell udev to forget your rule immediately
sudo udevadm control --reload-rules

# 5. Tell the kernel module loader to rebuild its index cleanly
sudo depmod -a
```

### Follow-up challenge (sequel?!)

Since I found this challenge very interesting, I decided to create a similar challenge for myself based on these concepts and call it task_5_sequel. If you are interested, feel free to view and solve that challenge on your journey of learning kernel development as well! :)

---

### A Quick Note on the Technical Edits:

The biggest technical adjustment I made was clarifying the difference between the kernel and `udev`. The kernel itself doesn't read the files in `/etc/udev/rules.d/`—the kernel just shouts "Hey, a USB was plugged in!" (a `uevent`), and a separate background program running in user-space (`systemd-udevd`) actually reads the rules folder and decides to run `modprobe`.

The "sequel" challenge you mentioned at the end sounds like a fantastic way to cement these concepts. What kind of twist did you add for your custom Task 5 sequel?

<details>

<summary><b>Wait?! Need a deeper dive into the "Dispatch" and "Catch" mechanics? Click here!</b></summary> 

If you remember, I said at the beginning "Whenever a device is added to or removed from a system, the kernel dispatches a hotplug event (known as a uevent) regarding that device. When this happens, user-space device managers like udev catch the event, iterate over the "rules" defined in the /etc/udev/rules.d/ directory, and take action based on the logic defined by each matched rule." If you wonder what dispatch and catch mean in terms of technical computer terms read the bottom! Heads up though: it will change the way you look at computers!

---

### The Heartbeat of the Machine: Interrupts

To understand how the kernel handles events, you first have to realize that the computer is not "thinking." It is reacting. The entire system is driven by **interrupts**.

There are two main ways the CPU is forced to stop what it's doing and talk to the kernel:

1. **The Heartbeat (Timer Interrupt):** Your motherboard has a physical crystal oscillator—a hardware stopwatch—that fires an electrical interrupt into the CPU roughly 1,000 times per second. This is the metronome of your OS. It pauses whatever you're doing, jumps into the kernel, and updates the math to see if your program has "run out of time."
2. **External Interrupts:** These are events like a USB plug-in, a mouse click, or a network packet. They are sudden, unpredictable electrical signals that force the CPU to stop and handle an external event *right now*.

### The Trap: The Exit-Path

When an interrupt (like a USB plug-in) finishes, the kernel doesn't just jump back to your program. It passes through a final check, known as the **exit-path**, to see if something more important needs the CPU. This is the exact moment where the scheduler takes the wheel:

```assembly
; This runs every single time ANY interrupt (USB, Timer, etc.) finishes
return_from_interrupt:

    ; The kernel checks: "Did anything happen that requires a task switch?"
    if (current_thread_info()->flags & TIF_NEED_RESCHED) {
        
        ; Clear the flag so we don't get stuck in a loop
        clear_bit(TIF_NEED_RESCHED);

        ; SWAP THE PROCESSES!
        ; The kernel consults its Red-Black Tree of tasks, picks the one 
        ; that deserves the CPU next (the one with the lowest vruntime),
        ; and context-switches to it.
        schedule(); 
    }

    ; If no switch was needed, we return to the user-space program 
    ; exactly where it left off.
    restore_registers_and_resume_userspace();

```

### The "Dispatch": Hardware reacting to Hardware

So, how does the kernel "dispatch" the event? When you plug in a USB device, the electrical voltage change hits the USB controller, which fires an **External Interrupt**. The CPU drops your user-space program, jumps into the kernel, and the kernel creates a text string (`uevent`) describing the new hardware.

The kernel then pushes this text into a **Netlink Socket**. This is a high-speed pipe that links the kernel to user-space.

### The "Catch": How `udev` wakes up

Here is the kicker: `udev` isn't checking for that text. It's not running a loop. It’s asleep.

When `udev` starts, it connects to that Netlink socket and calls a "blocking" function. The kernel marks `udev` as `TASK_INTERRUPTIBLE` (sleeping) and removes it from the CPU's run-queue. It consumes **0% CPU** while it waits.

When the kernel pushes the USB event text into the socket, it sees that `udev` is waiting for data there. It marks `udev` as `TASK_RUNNING`, sets the `TIF_NEED_RESCHED` flag (telling the scheduler "Hey, this is important!"), and finishes the USB interrupt.

As soon as that interrupt finishes, the kernel hits that `return_from_interrupt` logic, sees the `TIF_NEED_RESCHED` flag, calls `schedule()`, and **instantly jumps into the `udev` process**.

`udev` wakes up, reads the socket data, sees the USB event, executes your rule, and runs `modprobe`.

</details>