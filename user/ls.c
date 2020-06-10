#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
	static char buf[DIRSIZ+1];
	char *p;

	// Find first character after last slash.
	for(p=path+strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	if(strlen(p) >= DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
	return buf;
}

void
ls(char *path)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type){
	case T_FILE:
		printf("%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
		break;

	case T_DIR:
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("ls: cannot stat %s\n", buf);
				continue;
			}
			printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
		}
		break;
	}
	close(fd);
}

void int_to_bin_digit(unsigned int in, int count, int* out)
{
    unsigned int mask = 1U << (count-1);
    int i;
    for (i = 0; i < count; i++) {
        out[i] = (in & mask) ? 1 : 0;
        in <<= 1;
    }
}

void
lsl(char *path)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type){
	case T_FILE:
		printperm(st.mode, st.type == T_DIR);
		printf("%s %s %s %d %d %d\n", uidtouser(st.uid), gidtogroup(st.gid), fmtname(path), st.type, st.ino, st.size);
		break;

	case T_DIR:
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("ls: cannot stat %s\n", buf);
				continue;
			}

			printperm(st.mode, st.type == T_DIR);
			printf("%s %s %s %d %d %d\n", uidtouser(st.uid), gidtogroup(st.gid), fmtname(buf), st.type, st.ino, st.size);
		}
		break;
	}
	close(fd);
}

void
printperm(int mode, int dir)
{
	int bits[10];
	int_to_bin_digit(mode, 10, bits);

	int i;
	printf("%c", dir ? 'd' : '-');
	printf("%c", bits[1] ? 'r' : '-');
	printf("%c", bits[2] ? 'w' : '-');
	printf("%c", bits[3] ? 'x' : '-');
	printf("%c", bits[4] ? 'r' : '-');
	printf("%c", bits[5] ? 'w' : '-');
	printf("%c", bits[6] ? 'x' : '-');
	printf("%c", bits[7] ? 'r' : '-');
	printf("%c", bits[8] ? 'w' : '-');
	printf("%c", bits[9] ? 'x' : '-');
	printf(" ");
}

int
main(int argc, char *argv[])
{
	int i, l = 0;

	if(argc < 2){
		ls(".");
		exit();
	} else if (argc == 2 && strstr(argv[1], "-l") != NULL) {
		lsl(".");
		exit();
	}

	if (strstr(argv[1], "-l") != NULL)
		l = 1;

	for(i= l ? 2 : 1; i < argc; i ++) {
		if (l)
			lsl(argv[i]);
		else
			ls(argv[i]);
	}
	exit();
}
