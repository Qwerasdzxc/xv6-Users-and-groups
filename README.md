# Users and Groups system implementation

Unix systems are characterized by their support for multiple users. The xv6 system does not support multiple users. This projects implements multiple users within the xv6 system.

<p align="center">
  <img width="675" height="450" src="https://i.ibb.co/wNQydXR/Annotation-2020-06-10-164440.png">
</p>

Unix systems have user and user group concepts, and each file has a set of permissions (i.e., a mode) that describes which rights have which users and which user group members have over the file.

The code that deals with user management is divided into a part in the kernel and a part in the user space. In the kernel, users and groups are strictly numerical concepts. Each process has a UID (user id) and an EUID (effective user id) and a list of GID numbers, and each inode has a UID, GID, and mode parameter. The kernel does not track which users and which groups exist. In other words, all UID and GID numbers are valid. There is a library in the user space with routines for translating UIDs (ie GIDs) into text names of users (ie groups), and the user database is a text file /etc/passwd (and /etc/group for groups).

Each user has a symbolic name, typically identical UID and GID values, a real name (which is just secondary data), and the location of the home directory. Note that for each user there is a trivial group of the same name.

A user with UID 0 is special. It is typically called root (also called superuser in the literature). Root is the system administrator and can read and write from any file regardless of permissions (it cannot execute a program without execute permissions, but it can set execute permissions). In addition, only processes belonging to the root can make some system calls such as setuid() and chown().

The /etc directory is usually used for system configuration files, of which there are many on real systems. For the project, in addition to /etc/passwd and /etc/group, the files /etc/issue and /etc/motd have been added. They are plain text files that contain text that will be printed by the getty program. /etc/issue is printed before the "login:" message, and /etc/motd, ie. The "Message of The Day" is printed after the user successfully logs in, but before the shell is launched. Typically, the issue is a single line, and the motd is a short paragraph.

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern x86-based multiprocessor using ANSI C.

BUILDING AND RUNNING XV6

To build xv6 on an x86 ELF machine (like Linux or FreeBSD), run
"make". On non-x86 or non-ELF machines (like OS X, even on x86), you
will need to install a cross-compiler gcc suite capable of producing
x86 ELF binaries (see https://pdos.csail.mit.edu/6.828/).
Then run "make TOOLPREFIX=i386-jos-elf-". Now install the QEMU PC
simulator and run "make qemu".
