#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int userdone = 0;
int passdone = 0;

char username[30];
char pass[32];

int
getcmd(char *buf, int nbuf)
{
    if (!userdone) {
        printf("getty: Enter your username: ");
        gets(username, sizeof(username));
        username[strlen(username) - 1] = '\0';
        userdone = 1;
    } else {
        printf("getty: Enter current password: ");
        echoswitch();
        gets(pass, sizeof(pass));
        echoswitch();
        pass[strlen(pass) - 1] = '\0';
        passdone = 1;
    }

	return 0;
}


char*
uidtodir(uint uid)
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("uidtopass: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");

	int counter = 0;
	int found = 0;
	char* dir = NULL;

	while(tok != NULL) {
		// Read UID:
		if (counter == 2) {
			uint _uid = atoi(tok);
			if (_uid == uid) {
				found = 1;
			}
		}
		// Read pass:
		if (counter == 5) {
			if (found) {
				dir = malloc(strlen(tok)+1);
				strcpy(dir, tok);

				close(fd);
				return dir;
			}
		}

		if (counter == 4)
			tok = strtok(NULL, "\n");
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 6) {
			counter = 0;
		}
	}

	close(fd);

	return NULL;
}

int
main(int argc, char *argv[])
{
    int uid;
    int n;
    static char buf[100];
    char issues[256];

    clrscr();

    int fd;
	if ((fd = open("/etc/issue", 0)) < 0) {
		printf("getty: cannot open /etc/issue\n");
	}

	if ((n = read(fd, issues, sizeof(issues))) <= 0) {
        printf("getty: cannot read from /etc/issue\n");
    }

    printf("Issues: %s\n\n", issues);

	while(getcmd(buf, sizeof(buf)) >= 0){
        if (userdone && passdone) {

            uid = usertouid(username);
            if (uid < 0) {
                printf("getty: Username doesn't exist!\n");
                userdone = 0;
                passdone = 0;
                continue;
            }

            char* userpass = uidtopass(uid);
            if (strcmp(userpass, pass) != 0) {
                printf("getty: Passwords don't match!\n");
                userdone = 0;
                passdone = 0;
                continue;
            }

            break;
        }
		wait();
	}

    start(uid);
}

void start(int uid)
{    
    char *shargv[] = { "sh", 0 };
    int wpid;
    int n;
	char motd[256];

    int fd;
	if ((fd = open("/etc/motd", 0)) < 0) {
		printf("getty: cannot open /etc/motd\n");
		exit();
	}

	if ((n = read(fd, motd, sizeof(motd))) <= 0) {
        printf("getty: cannot read from /etc/motd\n");
		exit();
    }

    printf("\n%s", motd);

	close(fd);

    int pid = fork();
    if (pid < 0) {
        printf("getty: fork failed\n");
        exit();
    }
    if (pid == 0) {
        printf("getty: starting sh with uid %d\n", uid);

        int gids[1] = { uidtogid(uid) };
        setgroups(1, gids);
        setuid(uid);
        chdir(uidtodir(uid));
        exec("/bin/sh", shargv);
        printf("getty: exec sh failed\n");
        exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
        printf("zombie!\n");

	exit();
}