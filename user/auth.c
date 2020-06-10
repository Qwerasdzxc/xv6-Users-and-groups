#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"
#include "kernel/x86.h"

char*
uidtouser(uint uid)
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("uidtouser: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");

	int counter = 0;
	char* user = NULL;

	while(tok != NULL) {
		// Read user:
		if (counter == 0) {
			user = malloc(strlen(tok)+1);
			strcpy(user, tok);
		}
		// Read UID:
		if (counter == 2) {
			uint _uid = atoi(tok);
			if (_uid == uid) {
				close(fd);
				return user;
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

char*
gidtogroup(uint gid)
{
	int fd;

	if((fd = open("/etc/group", 0)) < 0){
		printf("gidtogroup: cannot open /etc/group\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");

	int counter = 0;
	char* group = NULL;

	while(tok != NULL) {

		// Read group:
		if (counter == 0) {
			group = malloc(strlen(tok)+1);
			strcpy(group, tok);
		}

		// Read GID:
		if (counter == 1) {
			uint _gid = atoi(tok);
			if (_gid == gid) {
				close(fd);
				return group;
			}
		}

		if (counter == 1)
			tok = strtok(NULL, "\n");
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 3) {
			counter = 0;
		}
	}

	close(fd);

	return NULL;
}

uint
nextuid()
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("nextuid: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;
	uint minuid = 1000;

	while(tok != NULL) {
		// Read UID:
		if (counter == 2) {
			int curruid = atoi(tok);

			if (curruid >= 1000 && curruid == minuid)
				minuid ++;
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

	return minuid;
}

uint
nextgid()
{
	int fd;

	if((fd = open("/etc/group", 0)) < 0){
		printf("nextuid: cannot open /etc/group\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;
	uint mingid = 1000;

	while(tok != NULL) {
		// Read GID:
		if (counter == 1) {
			int currgid = atoi(tok);

			if (currgid >= 1000 && currgid == mingid)
				mingid ++;
		}

		if (counter == 1)
			tok = strtok(NULL, "\n");
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 3) {
			counter = 0;
		}
	}

	close(fd);

	return mingid;
}

uint
validateuid(uint uid)
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("nextuid: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;

	while(tok != NULL) {
		// Read UID:
		if (counter == 2) {
			if (uid == atoi(tok)) {
				close(fd);
				return -1;
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

	return 0;
}

uint
validategid(uint gid)
{
	int fd;

	if((fd = open("/etc/group", 0)) < 0){
		printf("nextuid: cannot open /etc/group\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;

	while(tok != NULL) {
		// Read GID:
		if (counter == 1) {
			if (gid == atoi(tok)) {
				close(fd);
				return -1;
			}
		}

		if (counter == 1)
			tok = strtok(NULL, "\n");
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 3) {
			counter = 0;
		}
	}

	close(fd);

	return 0;
}

int
validateuserpass(uint uid, char* pass)
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("uidtouser: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");

	int counter = 0;
	char* _pass = NULL;

	while(tok != NULL) {
		// Read pass:
		if (counter == 1) {
			_pass = malloc(strlen(tok)+1);
			strcpy(_pass, tok);
		}
		// Read UID:
		if (counter == 2) {
			uint _uid = atoi(tok);
			if (_uid == uid) {
				int same = strcmp(pass, _pass);
				close(fd);
				return same == 0;
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

	return 0;
}

char*
uidtopass(uint uid)
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
	char* pass = NULL;

	while(tok != NULL) {
		// Read pass:
		if (counter == 1) {
			pass = malloc(strlen(tok)+1);
			strcpy(pass, tok);
		}
		// Read UID:
		if (counter == 2) {
			uint _uid = atoi(tok);
			if (_uid == uid) {
				close(fd);
				return pass;
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

uint
usertouid(char* name)
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

	while(tok != NULL) {
		// Read user:
		if (counter == 0) {
			if (strcmp(name, tok) == 0)
				found = 1;
		}
		// Read UID:
		if (counter == 2) {
			uint uid = atoi(tok);
			if (found) {
				close(fd);
				return uid;
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

	return -1;
}

uint
grouptogid(char* group)
{
	int fd;

	if((fd = open("/etc/group", 0)) < 0){
		printf("nextuid: cannot open /etc/group\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;
	int found = 0;

	while(tok != NULL) {
		// Read group:
		if (counter == 0) {
			if (strcmp(group, tok) == 0)
				found = 1;
		}

		if (counter == 1) {
			if (found) {
				close(fd);
				return atoi(tok);
			}
			tok = strtok(NULL, "\n");
		}
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 3) {
			counter = 0;
		}
	}

	close(fd);

	return -1;
}

uint
gidexists(uint gid)
{
	int fd;

	if((fd = open("/etc/group", 0)) < 0){
		printf("nextuid: cannot open /etc/group\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;

	while(tok != NULL) {
		// Read GID:
		if (counter == 1) {
			if (gid == atoi(tok)) {
				close(fd);
				return 1;
			}
		}

		if (counter == 1)
			tok = strtok(NULL, "\n");
		else
			tok = strtok(NULL, ":");

		counter ++;
		if (counter == 3) {
			counter = 0;
		}
	}

	close(fd);

	return -1;
}

uint
uidtogid(uint uid)
{
	int fd;

	if((fd = open("/etc/passwd", 0)) < 0){
		printf("nextuid: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	char* tok = strtok(buff, ":");
	int counter = 0;
	int found = 0;
	while(tok != NULL) {
		// Read UID:
		if (counter == 2) {
			if (uid == atoi(tok)) {
				found = 1;
			}
		}
		if (counter == 3) {
			if (found) {
				close(fd);
				return atoi(tok);
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

	return -1;
}

uint
passwdlen()
{
	int fd;
	if((fd = open("/etc/passwd", 0)) < 0){
		printf("passwdlen: cannot open /etc/passwd\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	close(fd);

	return strlen(buff);
}

uint
grouplen()
{
	int fd;
	if((fd = open("/etc/group", 0)) < 0){
		printf("grouplen: cannot open /etc/group\n");
		return NULL;
	}

	int n;
	char buff[2048];

	if ((n = read(fd, buff, sizeof(buff))) <= 0)
		return NULL;

	close(fd);

	return strlen(buff);
}