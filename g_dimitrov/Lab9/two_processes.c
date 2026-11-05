#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;
    int wait_child;

    if ((argc != 2 && argc != 3) ||
        (argc == 3 && strcmp(argv[2], "-w") != 0)) {
        fprintf(stderr, "Usage: %s file [-w]\n", argv[0]);
        return EXIT_FAILURE;
    }
    wait_child = (argc == 3);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        execl("/usr/bin/cat", "cat", argv[1], (char *)NULL);
        perror("execl");
        _exit(127);
    }

    printf("Parent: child PID = %ld\n", (long)pid);
    fflush(stdout);

    if (wait_child) {
        while (waitpid(pid, &status, 0) == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                return EXIT_FAILURE;
            }
        }
        printf("\nParent: last line (child has finished).\n");
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return EXIT_FAILURE;
    }

    printf("Parent: last line (without waiting).\n");

    return EXIT_SUCCESS;
}
