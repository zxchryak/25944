#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    pid = fork();
    switch (pid) {
        case -1:
            perror("fork");
            return EXIT_FAILURE;
        case 0:
            execvp(argv[1], &argv[1]);
            perror("execvp");
            _exit(127);
        default:
            while (waitpid(pid, &status, 0) == -1) {
                if (errno != EINTR) {
                    perror("waitpid");
                    return EXIT_FAILURE;
                }
            }
            if (WIFEXITED(status)) {
                printf("Exit code: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Terminated by signal: %d\n", WTERMSIG(status));
            }
            break;
    }
    return EXIT_SUCCESS;
}
