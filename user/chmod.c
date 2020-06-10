#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

#define ADD_ALL_READ  292
#define ADD_ALL_WRITE 146
#define ADD_ALL_EXEC  73

#define ADD_U_READ    256
#define ADD_U_WRITE   128
#define ADD_U_EXEC    64

#define ADD_G_READ    32
#define ADD_G_WRITE   16
#define ADD_G_EXEC    8

#define ADD_O_READ    4
#define ADD_O_WRITE   2
#define ADD_O_EXEC    1

#define REM_ALL_READ  219
#define REM_ALL_WRITE 365
#define REM_ALL_EXEC  438

#define REM_U_READ    255
#define REM_U_WRITE   383
#define REM_U_EXEC    447

#define REM_G_READ    479
#define REM_G_WRITE   495
#define REM_G_EXEC    503

#define REM_O_READ    507
#define REM_O_WRITE   509
#define REM_O_EXEC    510

int pow(int x, unsigned int y)
{
    if (y == 0)
        return 1;
    else if ((y % 2) == 0)
        return pow (x, y / 2) * pow (x, y / 2);
    else
        return x * pow (x, y / 2) * pow (x, y / 2);

}

long long octtodec(int oct)
{
    int dec = 0, i = 0;

    while(oct != 0)
    {
        dec += (oct%10) * pow(8,i);
        ++i;
        oct/=10;
    }

    i = 1;

    return dec;
}

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("chmod: invalid arg count\n");
        exit();
    }

    // chmod mode file ...
    if (strstr(argv[1], "+") == NULL && strstr(argv[1], "-") == NULL) {
        int mode = atoi(argv[1]);
        if (mode < 0) {
            printf("chmod: invalid mode\n");
            exit();
        }
        
        int i;
        for (i = 2; i < argc; i ++) {
            chmod(argv[i], octtodec(mode));
        }

        exit();
    }

    if (strlen(argv[1]) != 3) {
        printf("chmod: invalid mode format\n");
        exit();
    }

    char whom = argv[1][0];
    char symbol = argv[1][1];
    char perm = argv[1][2];

    if (whom != 'u' && whom != 'g' && whom != 'o' && whom != 'a') {
        printf("chmod: invalid whom\n");
        exit();
    }
    if (symbol != '+' && symbol != '-') {
        printf("chmod: invalid operator\n");
        exit();
    }
    if (perm != 'r' && perm != 'w' && perm != 'x') {
        printf("chmod: invalid permission\n");
        exit();
    }

    int i, fd, mode;
    struct stat st;
    for (i = 2; i < argc; i ++) {
        if((fd = open(argv[i], 0)) < 0) {
            printf("chmod: cannot open %s\n", argv[i]);
            exit();
	    }

        if(fstat(fd, &st) < 0){
            printf("chmod: cannot stat %s\n", argv[i]);
            close(fd);
            exit();
        }

        mode = st.mode;
        
        // adding read perm to all
        // 110 101 100
        //      OR
        // 100 100 100
        // -----------
        // 110 101 100

        // adding read perm to user
        // 110 101 100
        //      OR
        // 100 000 000
        // -----------
        // 110 101 100

        // removing read perm from all
        // 110 101 100
        //     AND
        // 011 011 011
        // -----------
        // 010 001 000

        // removing write perm from all
        // 110 101 100
        //     AND
        // 101 101 101
        // -----------
        // 010 001 100

        if (symbol == '+') {
            if (whom == 'a') {
                if (perm == 'r')
                    mode |= ADD_ALL_READ;
                else if (perm == 'w')
                    mode |= ADD_ALL_WRITE;
                else if (perm == 'x')
                    mode |= ADD_ALL_EXEC;
            } else if (whom == 'u') {
                if (perm == 'r')
                    mode |= ADD_U_READ;
                else if (perm == 'w')
                    mode |= ADD_U_WRITE;
                else if (perm == 'x')
                    mode |= ADD_U_EXEC;
            } else if (whom == 'g') {
                if (perm == 'r')
                    mode |= ADD_G_READ;
                else if (perm == 'w')
                    mode |= ADD_G_WRITE;
                else if (perm == 'x')
                    mode |= ADD_G_EXEC;
            } else if (whom == 'o') {
                if (perm == 'r')
                    mode |= ADD_O_READ;
                else if (perm == 'w')
                    mode |= ADD_O_WRITE;
                else if (perm == 'x')
                    mode |= ADD_O_EXEC;
            }
        } else if (symbol == '-') {
            if (whom == 'a') {
                if (perm == 'r')
                    mode &= REM_ALL_READ;
                else if (perm == 'w')
                    mode &= REM_ALL_WRITE;
                else if (perm == 'x')
                    mode &= REM_ALL_EXEC;
            } else if (whom == 'u') {
                if (perm == 'r')
                    mode &= REM_U_READ;
                else if (perm == 'w')
                    mode &= REM_U_WRITE;
                else if (perm == 'x')
                    mode &= REM_U_EXEC;
            } else if (whom == 'g') {
                if (perm == 'r')
                    mode &= REM_G_READ;
                else if (perm == 'w')
                    mode &= REM_G_WRITE;
                else if (perm == 'x')
                    mode &= REM_G_EXEC;
            } else if (whom == 'o') {
                if (perm == 'r')
                    mode &= REM_O_READ;
                else if (perm == 'w')
                    mode &= REM_O_WRITE;
                else if (perm == 'x')
                    mode &= REM_O_EXEC;
            }
        }

        chmod(argv[i], mode);
    }
    
	exit();
}