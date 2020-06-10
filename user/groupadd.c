#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

#define STATUS_UNDEFINED 0
#define STATUS_GID 1
#define STATUS_GROUP 2

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("groupadd: group is required\n");
        exit();
    }

	int gid = -1, groupfd, i;
    char dir[64];
    char *group;

    int status = STATUS_UNDEFINED;

    for (i = 1; i < argc; i ++)
    {
        char *currArg = argv[i];
        // Expecting status token or group:
        if (status == STATUS_UNDEFINED) {
            if (strstr(currArg, "-g"))
                status = STATUS_GID;
            else if (strstr(currArg, "-")) {
                printf("groupadd: invalid command\n");
                exit();
            }
            else
                group = currArg;

        } else {
            if (status == STATUS_GID)
                gid = atoi(currArg);

            status = STATUS_UNDEFINED;
        }
    }

    if (group == NULL) {
        printf("groupadd: group is required\n");
        exit();
    }
    if((groupfd = open("/etc/group", O_RDWR)) < 0){
		printf("groupadd: cannot open etc/group\n");
		exit();
	}

    // UID validation:
    if (gid == -1)
        gid = nextgid();
    else if (validategid(gid) < 0) {
        printf("groupadd: gid already exists\n");
		exit();
    }

    char groupbuff[256];
    char _groupgid[10];

    strcat(groupbuff, group);
    strcat(groupbuff, ":");
    strcat(groupbuff, itoa(gid, _groupgid, 10));
    strcat(groupbuff, ":");
    strcat(groupbuff, " \n\0");

    // Reading the file just to move offset to the end:
    read(groupfd, 0, grouplen());

    if ((write(groupfd, groupbuff, strlen(groupbuff))) <= 0) {
        printf("useradd: cannot write into group\n");
		exit();
    }

    close(groupfd);
	exit();
}