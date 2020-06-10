#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    char* user;
    char currpass[32];
    int passfd;
    uint uid = getuid();

    if (argc > 1) {
        if (uid != 0) {
            printf("passwd: only root can change other user's passwords\n");
            exit();
        }
        else
            user = argv[1];
    } else
        user = uidtouser(uid);

    if (uid != 0) {
        printf("Enter current password: ");
        echoswitch();
        gets(currpass, sizeof(currpass));
        echoswitch();
        currpass[strlen(currpass) - 1] = '\0';
        if (!validateuserpass(uid, currpass)) {
            printf("passwd: password incorrect\n");
            exit();
        }
    } else
        strcpy(currpass, uidtopass(usertouid(user)));

    char pass[32];
    printf("Enter new password: ");
    echoswitch();
    gets(pass, sizeof(pass));
    echoswitch();
    pass[strlen(pass) - 1] = '\0';
    if (strlen(pass) < 7) {
        printf("passwd: password must be at least 7 characters long\n");
        exit();
    }

    char reppass[32];
    printf("Enter password again: ");
    echoswitch();
    gets(reppass, sizeof(reppass));
    echoswitch();
    reppass[strlen(reppass) - 1] = '\0';
    if (strlen(pass) != strlen(reppass) || strcmp(pass, reppass) != 0) {
        printf("passwd: passwords dont't match\n");
        exit();
    }

    if((passfd = open("/etc/passwd", O_RDWR)) < 0){
		printf("passwd: cannot open /etc/passwd\n");
		exit();
	}

	int n;
	char buff[512];

	if ((n = read(passfd, buff, sizeof(buff))) <= 0) {
        printf("passwd: cannot read from /etc/passwd\n");
		exit();
    }

    char srchbuff[256];
    strcat(srchbuff, user);
    strcat(srchbuff, ":");

    char* indexptr = strstr(buff, srchbuff);
    int index = indexptr - buff;
    int start = index + strlen(user) + 1;
    int end = start + strlen(currpass);

    int i;
    for (i = 0; i < strlen(currpass); i ++) {
        memmove(&buff[start], &buff[start + 1], strlen(buff) - start);
    }

    char file[512];
    strncpy(file, buff, start);
    file[start] = '\0';
    strcat(file, pass);
    strcat(file, buff + start);
    file[strlen(file)] = '\0';
    close(passfd);

    if((passfd = open("/etc/passwd", O_RDWR)) < 0){
		printf("passwd: cannot open /etc/passwd\n");
		exit();
	}

    if ((write(passfd, file, strlen(file))) <= 0) {
        printf("passwd: cannot write into passwd\n");
		exit();
    }

    int diff = strlen(currpass) - strlen(pass);
    
    // Clearing leftover characters
    if (diff > 0) {
        char zeros[diff];
        for (i = 0; i < diff; i ++)
            zeros[i] = 0;
        zeros[0] = '\0';
        write(passfd, zeros, diff);
    }

    close(passfd);
	exit();
}