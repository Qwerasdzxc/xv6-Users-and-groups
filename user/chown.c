#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("chown: invalid arg count\n");
        exit();
    }

    int i;
    char* user = NULL;
    char* group = NULL;

    if (strlen(argv[1]) <= 1) {
        printf("chown: invalid args\n");
        exit();
    }

    // No colon so only user was provided:
    if (strstr(argv[1], ":") == NULL) {
        user = argv[1];
    }
    // Colon is the first arg so we're expecting group: 
    else if (strncmp(argv[1], ":", 1) == 0) {
        group = argv[1];
        // Removing the semicolon from start
        memmove(group, group+1, strlen(group));
    } 
    
    else {
        int arglen = strlen(argv[1]);
        char* token = strtok(argv[1], ":");

        // User and colon after him:
        if (strlen(token) + 1 == arglen) {
            user = token;
            group = gidtogroup(uidtogid(usertouid(user)));
        } 
        // We have both the user and the group:
        else {
            user = token;
            token = strtok(NULL, ":");
            group = token;
        }
    }

    uint uid = -1, gid = -1;
    if (user != NULL) {
        uid = atoi(user);

        if (uid == -1)
            uid = usertouid(user);
    }
    if (group != NULL) {
        gid = atoi(group);

        if (gid == -1)
            gid = grouptogid(group);
    }

    for (i = 2; i < argc; i ++)
        chown(argv[i], uid, gid);
    
	exit();
}