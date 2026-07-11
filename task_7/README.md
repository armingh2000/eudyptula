# Task 7

# Challenge

Great work with that misc device driver.  Isn't that a nice and simple
way to write a character driver?

Just when you think this challenge is all about writing kernel code,
this task is a throwback to your second one.  Yes, that's right,
building kernels.  Turns out that's what most developers end up doing,
tons and tons of rebuilds, not writing new code.  Sad, but it is a good
skill to know.

The tasks this round are:
  - Download the linux-next kernel for today.  Or tomorrow, just use
    the latest one.  It changes every day so there is no specific one
    you need to pick.  Build it.  Boot it.  Provide proof that you built
    and booted it.

What is the linux-next kernel?  Ah, that's part of the challenge.

For a hint, you should read the excellent documentation about how the
Linux kernel is developed in Documentation/development-process/ in the
kernel source itself.  It's a great read, and should tell you all you
never wanted to know about what Linux kernel developers do and how they
do it.

As always, please respond to this challenge with your id.  I know you
know what it is.  I'll not even include it this time, I trust you.
Don't make me feel that is a mistake.

# Solution

To understand exactly what the `linux-next` tree is and why it exists, I started by reading the [Linux Kernel Development Process documentation](https://docs.kernel.org/process/2.Process.html). Essentially, `linux-next` acts as the testing ground for patches destined for the next merge window. It provides a consolidated view of what is coming down the pipeline, which helps identify conflicts and bugs early.

With that context, the workflow is quite similar to Task 2. I downloaded the latest `linux-next` source, configured it, and built it from source.

Once the compilation finished and I booted the resulting image in QEMU, I verified the build using `uname -r`. The output confirms I am running the latest `linux-next` kernel:

`7.2.0-rc2-next-20260710`
