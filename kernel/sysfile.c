//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "x86.h"
#include "memlayout.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
	int fd;
	struct file *f;

	if(argint(n, &fd) < 0)
		return -1;
	if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
		return -1;
	if(pfd)
		*pfd = fd;
	if(pf)
		*pf = f;
	return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
	int fd;
	struct proc *curproc = myproc();

	for(fd = 0; fd < NOFILE; fd++){
		if(curproc->ofile[fd] == 0){
			curproc->ofile[fd] = f;
			return fd;
		}
	}
	return -1;
}

int
sys_getuid(void)
{
	return myproc()->uid;
}

int
sys_geteuid(void)
{
	return myproc()->euid;
}

int
sys_setuid(void)
{
	if (myproc()->uid != 0)
		return -1;

	int uid;

	argint(0, &uid);
	
	myproc()->uid = uid;
	myproc()->euid = uid;
}

int
sys_setgroups(void)
{
	if (myproc()->uid != 0)
		return -1;
	
	int ngroups;
	int* rgids;

	if(argint(0, &ngroups) < 0)
		return -1;

	if(argptr(1, (int*)&rgids, 2*sizeof(rgids[0])) < 0)
		return -1;

	int i;
	for (i = 0; i < ngroups; i ++) {
		myproc()->gids[i] = rgids[i];
	}

	int j;
	for (j = i; j < 32; j ++) {
		myproc()->gids[j] = -1;
	}

	return 0;
}

int
sys_chmod(void)
{
	char *path;
	int mode;
	struct inode *ip;

	if(argstr(0, &path) < 0 || argint(1, &mode) < 0)
		return -1;

	begin_op();

	if((ip = namei(path)) == 0) {
		end_op();
		return -1;
	}

	if (ip->uid != myproc()->uid && myproc()->uid != 0) {
		end_op();
		return -1;
	}

	ilock(ip);

	ip->mode = mode;

	iupdate(ip);
	iunlock(ip);
	end_op();

	return 0;
}

int
sys_chown(void)
{
	if (myproc()->uid != 0)
		return -1;

	char *path;
	int owner, group;
	struct inode *ip;

	if(argstr(0, &path) < 0 || argint(1, &owner) < 0 || argint(2, &group) < 0)
		return -1;

	begin_op();

	if((ip = namei(path)) == 0) {
		end_op();
		return -1;
	}

	ilock(ip);

	if (owner != -1)
		ip->uid = owner;

	ip->gid = group;

	iupdate(ip);
	iunlock(ip);
	end_op();
	return 0;
}

uint echodisabled;

int
sys_echoswitch(void)
{
	echodisabled = !echodisabled;

	return 0;
}

#define CRTPORT 0x3d4

void
setcp(int x, int y)
{
	int pos = x + 80 * y;

	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
}

int
sys_clrscr(void)
{
	ushort *crt = (ushort*)P2V(0xb8000);
	int i;

	for (i = 0; i < 24 * 80; i ++)
		crt[i] = 0;

	setcp(0, 0);
}

int
sys_dup(void)
{
	struct file *f;
	int fd;

	if(argfd(0, 0, &f) < 0)
		return -1;
	if((fd=fdalloc(f)) < 0)
		return -1;
	filedup(f);
	return fd;
}

int
sys_read(void)
{
	struct file *f;
	int n;
	char *p;

	if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
		return -1;

	return fileread(f, p, n);
}

int
sys_write(void)
{
	struct file *f;
	int n;
	char *p;

	if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
		return -1;
	return filewrite(f, p, n);
}

int
sys_close(void)
{
	int fd;
	struct file *f;

	if(argfd(0, &fd, &f) < 0)
		return -1;
	myproc()->ofile[fd] = 0;
	fileclose(f);
	return 0;
}

int
sys_fstat(void)
{
	struct file *f;
	struct stat *st;

	if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
		return -1;
	
	return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
	char name[DIRSIZ], *new, *old;
	struct inode *dp, *ip;

	if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
		return -1;

	begin_op();
	if((ip = namei(old)) == 0){
		end_op();
		return -1;
	}

	ilock(ip);
	if(ip->type == T_DIR){
		iunlockput(ip);
		end_op();
		return -1;
	}

	ip->nlink++;
	iupdate(ip);
	iunlock(ip);

	if((dp = nameiparent(new, name)) == 0)
		goto bad;
	ilock(dp);
	if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
		iunlockput(dp);
		goto bad;
	}
	iunlockput(dp);
	iput(ip);

	end_op();

	return 0;

bad:
	ilock(ip);
	ip->nlink--;
	iupdate(ip);
	iunlockput(ip);
	end_op();
	return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
	int off;
	struct dirent de;

	for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
		if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
			panic("isdirempty: readi");
		if(de.inum != 0)
			return 0;
	}
	return 1;
}

int
sys_unlink(void)
{
	struct inode *ip, *dp;
	struct dirent de;
	char name[DIRSIZ], *path;
	uint off;

	if(argstr(0, &path) < 0)
		return -1;

	begin_op();
	if((dp = nameiparent(path, name)) == 0){
		end_op();
		return -1;
	}

	ilock(dp);

	// Cannot unlink "." or "..".
	if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
		goto bad;

	if((ip = dirlookup(dp, name, &off)) == 0)
		goto bad;
	ilock(ip);

	if(ip->nlink < 1)
		panic("unlink: nlink < 1");
	if(ip->type == T_DIR && !isdirempty(ip)){
		iunlockput(ip);
		goto bad;
	}

	memset(&de, 0, sizeof(de));
	if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
		panic("unlink: writei");
	if(ip->type == T_DIR){
		dp->nlink--;
		iupdate(dp);
	}
	iunlockput(dp);

	ip->nlink--;
	iupdate(ip);
	iunlockput(ip);

	end_op();

	return 0;

bad:
	iunlockput(dp);
	end_op();
	return -1;
}

static struct inode*
create(char *path, short type, short major, short minor, uint uid, uint mode)
{
	struct inode *ip, *dp;
	char name[DIRSIZ];

	if((dp = nameiparent(path, name)) == 0)
		return 0;
	ilock(dp);

	if((ip = dirlookup(dp, name, 0)) != 0){
		iunlockput(dp);
		ilock(ip);
		if((type == T_FILE && ip->type == T_FILE) || ip->type == T_DEV)
			return ip;
		iunlockput(ip);
		return 0;
	}

	if((ip = ialloc(dp->dev, type, uid, mode)) == 0)
		panic("create: ialloc");

	ilock(ip);
	ip->major = major;
	ip->minor = minor;
	ip->nlink = 1;
	iupdate(ip);

	if(type == T_DIR){  // Create . and .. entries.
		dp->nlink++;  // for ".."
		iupdate(dp);
		// No ip->nlink++ for ".": avoid cyclic ref count.
		if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
			panic("create dots");
	}

	if(dirlink(dp, name, ip->inum) < 0)
		panic("create: dirlink");

	iunlockput(dp);

	return ip;
}

#define USER_R_BIT   1
#define USER_W_BIT   2
#define USER_X_BIT   3
#define GROUP_R_BIT  4
#define GROUP_W_BIT  5
#define GROUP_X_BIT  6
#define OTHER_R_BIT  7
#define OTHER_W_BIT  8
#define OTHER_X_BIT  9

int
sys_open(void)
{
	char *path;
	int fd, omode;
	struct file *f;
	struct inode *ip;

	if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
		return -1;

	begin_op();

	if (omode & O_CREATE){
		ip = create(path, T_FILE, 0, 0, myproc()->uid, 0644);
		if(ip == 0){
			end_op();
			return -1;
		}
	}

	if((ip = namei(path)) == 0){
		end_op();
		return -1;
	}

	ilock(ip);

	if(ip->type == T_DIR && omode != O_RDONLY){
		iunlockput(ip);
		end_op();
		return -1;
	}
		
	int bits[10];
	int_to_bin_digit(ip -> mode, 10, bits);
	int hasperm = 0;

	struct proc *currproc = myproc();

	int fileowner = ip -> uid == currproc -> euid;
	int groupmember = 0;

	int other = fileowner != 1 && groupmember != 1;
	int invalid = 0;

	if (omode == O_RDWR) {
		if (fileowner) {
			if (!bits[USER_R_BIT] || !bits[USER_W_BIT])
				invalid = 1;
		} else if (groupmember) {
			if (!bits[GROUP_R_BIT] || !bits[GROUP_W_BIT])
				invalid = 1;
		}
		else if (other) {
			if (!bits[OTHER_R_BIT] || !bits[GROUP_W_BIT])
				invalid = 1;
		}
	} else if (omode == O_RDONLY) {
		if (fileowner) {
			if (!bits[USER_R_BIT])
				invalid = 1;
		} else if (groupmember) {
			if (!bits[GROUP_R_BIT])
				invalid = 1;
		}
		else if (other) {
			if (!bits[OTHER_R_BIT])
				invalid = 1;
		}
	} else if (omode == O_WRONLY) {
		if (fileowner) {
			if (!bits[USER_W_BIT])
				invalid = 1;
		} else if (groupmember) {
			if (!bits[GROUP_W_BIT])
				invalid = 1;
		}
		else if (other) {
			if (!bits[OTHER_W_BIT])
				invalid = 1;
		}
	}

	// If we're root, invalid is not important
	invalid = currproc -> euid == 0 ? 0 : invalid;

	if (invalid) {
		cprintf("%s: Permission denied\n", path);
		iunlockput(ip);
		end_op();
		return -1;
	}

	if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
		if(f)
			fileclose(f);

		iunlockput(ip);
		end_op();
		return -1;
	}

	iunlock(ip);
	end_op();

	f->type = FD_INODE;
	f->ip = ip;
	f->off = 0;
	f->readable = !(omode & O_WRONLY);
	f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
	return fd;
}

int
sys_mkdir(void)
{
	char *path;
	struct inode *ip;

	begin_op();
	if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0, myproc()->uid, 0700)) == 0){
		end_op();
		return -1;
	}
	iunlockput(ip);
	end_op();
	return 0;
}

int
sys_mknod(void)
{
	struct inode *ip, *parent;
	char *path;
	char name[DIRSIZ];
	int major, minor;

	parent = nameiparent(path, name);

	int bits[10];
	int_to_bin_digit(parent -> mode, 10, bits);
	int hasperm = 0;

	int fileowner = parent -> uid == myproc() -> euid;
	int groupmember = 0;

	int other = fileowner != 1 && groupmember != 1;
	int invalid = 0;

	if (fileowner) {
		if (!bits[USER_W_BIT])
			invalid = 1;
	} else if (groupmember) {
		if (!bits[GROUP_W_BIT])
			invalid = 1;
	}
	else if (other) {
		if (!bits[OTHER_W_BIT])
			invalid = 1;
	}

	// If we're root, invalid is not important
	invalid = myproc() -> euid == 0 ? 0 : invalid;

	if (invalid) {
		cprintf("%s: Permission denied\n", path);
		iunlockput(ip);
		end_op();
		return -1;
	}

	begin_op();
	if((argstr(0, &path)) < 0 ||
			argint(1, &major) < 0 ||
			argint(2, &minor) < 0 ||
			(ip = create(path, T_DEV, major, minor, myproc()->uid, 0644)) == 0){
		end_op();
		return -1;
	}
	iunlockput(ip);
	end_op();
	return 0;
}

int
sys_chdir(void)
{
	char *path;
	struct inode *ip;
	struct proc *curproc = myproc();

	begin_op();
	if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
		end_op();
		return -1;
	}
	ilock(ip);
	if(ip->type != T_DIR){
		iunlockput(ip);
		end_op();
		return -1;
	}

	int bits[10];
	int_to_bin_digit(ip -> mode, 10, bits);
	int hasperm = 0;

	int fileowner = ip -> uid == curproc -> euid;
	int groupmember = 0;

	int other = fileowner != 1 && groupmember != 1;
	int invalid = 0;

	if (fileowner) {
		if (!bits[USER_X_BIT])
			invalid = 1;
	} else if (groupmember) {
		if (!bits[GROUP_X_BIT])
			invalid = 1;
	}
	else if (other) {
		if (!bits[OTHER_X_BIT])
			invalid = 1;
	}

	// If we're root, invalid is not important
	invalid = curproc -> euid == 0 ? 0 : invalid;

	if (invalid) {
		cprintf("%s: Permission denied\n", path);
		iunlockput(ip);
		end_op();
		return -1;
	}

	iunlock(ip);
	iput(curproc->cwd);
	end_op();
	curproc->cwd = ip;
	return 0;
}

int
sys_exec(void)
{
	char *path, *argv[MAXARG];
	int i;
	uint uargv, uarg;

	if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
		return -1;
	}
	memset(argv, 0, sizeof(argv));
	for(i=0;; i++){
		if(i >= NELEM(argv))
			return -1;
		if(fetchint(uargv+4*i, (int*)&uarg) < 0)
			return -1;
		if(uarg == 0){
			argv[i] = 0;
			break;
		}
		if(fetchstr(uarg, &argv[i]) < 0)
			return -1;
	}
	return exec(path, argv);
}

int
sys_pipe(void)
{
	int *fd;
	struct file *rf, *wf;
	int fd0, fd1;

	if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
		return -1;
	if(pipealloc(&rf, &wf) < 0)
		return -1;
	fd0 = -1;
	if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
		if(fd0 >= 0)
			myproc()->ofile[fd0] = 0;
		fileclose(rf);
		fileclose(wf);
		return -1;
	}
	fd[0] = fd0;
	fd[1] = fd1;
	return 0;
}
