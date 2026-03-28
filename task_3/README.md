# Task 3

## Challenge

Now that you have your custom kernel up and running, it's time to modify it!

The tasks for this round is:
  - take the kernel git tree from Task 02 and modify the Makefile to and modify the EXTRAVERSION field.  Do this in a way that the running kernel (after modifying the Makefile, rebuilding, and rebooting) has the characters "-eudyptula" in the version string.
  - show proof of booting this kernel.  Extra cookies for you by providing creative examples, especially if done in intrepretive dance at your local pub.
  - Send a patch that shows the Makefile modified.  Do this in a manner that would be acceptable for merging in the kernel source tree. (Hint, read the file Documentation/SubmittingPatches and follow the steps there.)

Remember to use your ID assigned to you in the Subject: line when responding to this task, so that I can figure out who to attribute it to.

If you forgot, your id is "X".  Surely I don't need to keep saying this right?  I know, _you_ wouldn't forget, but someone else, of course they would, so I'll just leave it here for those "others".

## Solution
We start by modifying the Makefile and then we create a patch and mail it to ourselves. We will also install `neomutt` and connect it to our Gmail to view the patch.

### Adjust Makefile
We only need to set the `EXTRAVERSION` field at the top of the Makefile to "-eudyptula". Once this is done, we need to create our patch based on this single change.

`EXTRAVERSION = -eudyptula`

We can build and run the kernel using the steps mentioned in Task 2. Once the kernel is running, we can use the `uname -r` command to see the kernel version string:

```bash
$ uname -r
6.19.0-eudyptula-g2ecc076ae07a
```
*(Note: If your version ends in `-dirty`, ensure you have committed all changes before building.)*

### Create the Patch
I suggest you first read [Patch Philosophy](https://kernelnewbies.org/PatchPhilosophy), [First Kernel Patch](https://kernelnewbies.org/FirstKernelPatch), and the [Submitting Patches](https://docs.kernel.org/process/submitting-patches.html) documentation as they help a lot to understand what a patch is and how to submit one.

**What is a Patch?**
A patch is a text file containing the commit metadata (author, date), a subject line, a description, and the "diff" (the actual code changes).

**Create a Patch**
To create the patch, we first need to commit our changes to the Makefile. Our commits must be signed off by us. Hence, we first need to tell git who we are:

```bash
git config --global user.email "Your Email"
git config --global user.name "Your Full Name"
```

Once we have our identity set, we can continue to create our commit:

`git commit -s -v`

For the commit message, we should follow the format explained in the mentioned documentation:

> Makefile: Set EXTRAVERSION to -eudyptula
>
> Set EXTRAVERSION in top-level Makefile to include "-eudyptula" suffix.

We don't need to manually put `[PATCH]` in our commit subject; once we export it into a Patch file, Git will automatically add the `[PATCH]` prefix.

To export our patch, we can use:
`git format-patch --subject-prefix="PATCH X" -o /tmp/ HEAD^`

This tells Git to look at the difference between the parent and the current commit. Effectively, it says: "Give me a patch for the very last commit I just made." If we were to make 3 commits, we could use `git format-patch -o /tmp/ HEAD~3` instead. We have also declared the subject prefix here to include `X`, as requested by the challenge. In reality, running it without the `--subject-prefix` will only add the `[PATCH]` prefix, which is usually enough for standard submissions.

Now we have our patch file in the `/tmp/` folder and we can view it:

```text
From 2ecc076ae07a944589c66f0d758171790478d722 Mon Sep 17 00:00:00 2001
From: ...
Date: ...
Subject: [PATCH X] Makefile: Set EXTRAVERSION to -eudyptula

Set EXTRAVERSION in top-level Makefile to include "-eudyptula" suffix.

Signed-off-by: ...
---
 Makefile | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)

diff --git a/Makefile b/Makefile
index 13107aa0a5fb1..dceb9422a20a4 100644
--- a/Makefile
+++ b/Makefile
@@ -2,7 +2,7 @@
 VERSION = 6
 PATCHLEVEL = 19
 SUBLEVEL = 0
-EXTRAVERSION =
+EXTRAVERSION = -eudyptula
 NAME = Baby Opossum Posse
 
 # *DOCUMENTATION*
-- 
2.53.0
```

Now that we have the patch ready, we can email it to ourselves (in the real world, we would email it to the maintainers list retrieved by `perl scripts/get_maintainer.pl -f Makefile`).

**Bonus: Check Patch (Optional)**
Once we have the patch ready, we can verify it using the `checkpatch.pl` script in the source tree:

`./scripts/checkpatch.pl /tmp/0001-Makefile-Set-EXTRAVERSION-to-eudyptula.patch`

If our patch has errors/warnings, this tool helps us identify and address them. However, we must still check the patch against policies ourselves as the tool is not 100% exhaustive.

### Git Send-Email
There are multiple ways to email a patch. Here we will use `git send-email`.

**Install Git Send-Email (Fedora)**

`sudo dnf install git-email`

**Configure Git Send-Email**

We can run the following commands to configure our client:

```bash
git config --global sendemail.smtpserver smtp.gmail.com
git config --global sendemail.smtpserverport 587
git config --global sendemail.smtpencryption tls
git config --global sendemail.smtpuser YourEmail
```

**Get App Password (For Gmail)**
Before sending, we need an **App Password** from Google so that the git client can authenticate. We can set this up by visiting [App Passwords](https://myaccount.google.com/apppasswords).

**Send the Email**

Sending the email is straightforward:

```bash
git send-email --to="Your aEmail" --annotate /tmp/0001-Makefile-Set-EXTRAVERSION-to-eudyptula.patch
```

Because we used `--annotate`, an editor will open. Here we can include our "proof of booting" after the `---` line (which sits below the `Signed-off-by` line). This ensures the proof is in the email but **doesn't** become part of the permanent git history:

```text
...
Signed-off-by: ...
---
Proof of booting:
$ uname -r
6.19.0-eudyptula-g2ecc076ae07a

 Makefile | 2 +-
...
```

Now we can check our email and verify that the patch was sent correctly.

### Check the Email Using a Terminal Mail Client 

**NeoMutt Setup**
NeoMutt is a command-line mail reader (MUA). To install it on Fedora:

`sudo dnf install neomutt`

We need to create a config file:
```bash
mkdir -p ~/.config/neomutt
touch ~/.config/neomutt/neomuttrc
```

We can use the following configuration for a quick setup:

```muttrc
# --- User Identity ---
set realname = "Your Full Name"
set from = "Your Email"
set use_from = yes

# --- Gmail IMAP/SMTP Setup ---
set imap_user = "Your Email"
set imap_pass = "" # NeoMutt will prompt for your App Password

set smtp_url = "smtp://YourEmail@smtp.gmail.com:587/"
set smtp_pass = ""

# --- Folders ---
set folder = "imaps://imap.gmail.com:993"
set spoolfile = "+INBOX"

# --- Editor ---
set editor = "nvim"
set edit_headers = yes
set text_flowed = no # CRITICAL: Prevents Gmail from mangling formatting

# --- Interface ---
set sort = reverse-threads
set sort_aux = last-date-received
set wait_key = no
```

Once set up, we run `neomutt` and enter our App Password to access our emails.

**Why use NeoMutt and not the Gmail web UI?**

The Gmail web interface often "helps" by wrapping long lines, forcing HTML, or changing tabs to spaces. This is unacceptable for kernel patches as it breaks the code formatting. Using a purely text-based client like `neomutt` ensures our patch remains bit-perfect and easy for maintainers to apply.
