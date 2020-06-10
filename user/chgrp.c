#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("chgrp: invalid arg count\n");
        exit();
    }

    char* arggroup = argv[1];
    uint gid = atoi(arggroup);
    int i;

    if (gid == -1)
        gid = grouptogid(arggroup);
    if (gid == -1 || gidexists(gid) == -1) {
        printf("chgrp: invalid gid\n");
        exit();
    }

    for (i = 2; i < argc; i ++)
        chown(argv[i], -1, gid);
    
	exit();
}