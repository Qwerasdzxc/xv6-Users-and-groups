#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

#define STATUS_UNDEFINED 0
#define STATUS_DIR 1
#define STATUS_UID 2
#define STATUS_NAME 3

void
insertstring(char* destination, int pos, char* seed)
{
    char * strC;

    strC = (char*)malloc(strlen(destination)+strlen(seed)+1);
    strncpy(strC,destination,pos);
    strC[pos] = '\0';
    strcat(strC,seed);
    strcat(strC,destination+pos);
    strcpy(destination,strC);
    free(strC);
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("useradd: user is required\n");
        exit();
    }

	int uid = -1, gid, passfd, groupfd, i;
    char dir[64];
    char *name, *user;

    int status = STATUS_UNDEFINED;

    for (i = 1; i < argc; i ++)
    {
        char *currArg = argv[i];
        // Expecting status token or username:
        if (status == STATUS_UNDEFINED) {
            if (strstr(currArg, "-d"))
                status = STATUS_DIR;
            else if (strstr(currArg, "-u"))
                status = STATUS_UID;
            else if (strstr(currArg, "-c"))
                status = STATUS_NAME;
            else if (strstr(currArg, "-")) {
                printf("useradd: invalid command\n");
                exit();
            }
            else
                user = currArg;

        } else {
            if (status == STATUS_DIR)
                strcpy(dir, currArg);
            else if (status == STATUS_UID)
                uid = atoi(currArg);
            else if (status == STATUS_NAME)
                name = currArg;

            status = STATUS_UNDEFINED;
        }
    }

    if (user == NULL) {
        printf("useradd: user is required\n");
        exit();
    }
    if((passfd = open("/etc/passwd", O_RDWR)) < 0){
		printf("useradd: cannot open etc/passwd\n");
		exit();
	}
    if((groupfd = open("/etc/group", O_RDWR)) < 0){
		printf("useradd: cannot open etc/group\n");
		exit();
	}

    // UID validation:
    if (uid == -1)
        uid = nextuid();
    else if (validateuid(uid) < 0) {
        printf("useradd: uid already exists\n");
		exit();
    }

    // GID creation:
    gid = nextgid();

    // Directory name building:
    if (strlen(dir) == 0) {
        strcat(dir, "/home/");
        strcat(dir, user);
        strcat(dir, "\0");
    } else
        insertstring(dir, 0, "/home/\0");
    
    if (mkdir(dir) < 0) {
        printf("useradd: cannot create user directory\n");
		exit();
    }

    chown(dir, uid, gid);

	int n;
	char passbuff[256];
    char _uid[10];
    char _passgid[10];

    strcat(passbuff, user);
    strcat(passbuff, ":_:");
    strcat(passbuff, itoa(uid, _uid, 10));
    strcat(passbuff, ":");
    strcat(passbuff, itoa(gid, _passgid, 10));
    strcat(passbuff, ":");
    strcat(passbuff, name == NULL ? "_" : name);
    strcat(passbuff, ":");
    strcat(passbuff, dir);
    strcat(passbuff, "\n\0");

    // Reading the file just to move offset to the end:
    read(passfd, 0, passwdlen());
    
    if ((write(passfd, passbuff, strlen(passbuff))) <= 0) {
        printf("useradd: cannot write into passwd\n");
		exit();
    }

    char groupbuff[256];
    char _groupgid[10];

    strcat(groupbuff, user);
    strcat(groupbuff, ":");
    strcat(groupbuff, itoa(gid, _groupgid, 10));
    strcat(groupbuff, ":");
    strcat(groupbuff, user);
    strcat(groupbuff, "\n\0");

    // Reading the file just to move offset to the end:
    read(groupfd, 0, grouplen());

    if ((write(groupfd, groupbuff, strlen(groupbuff))) <= 0) {
        printf("useradd: cannot write into group\n");
		exit();
    }

    close(passfd);
    close(groupfd);
	exit();
}