#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }
    switch (fork()) {
        case -1:
            perror("fork");
            return EXIT_FAILURE;
        case 0:
            execlp("cat", "cat", argv[1], (char *)NULL);
            perror("execlp");
            _exit(127);
        default:
            if (wait(NULL) == -1) {
                perror("wait");
                return EXIT_FAILURE;
            }
            printf("\nChild finished. Parent process.\n");
            break;
    }
    return EXIT_SUCCESS;
}
