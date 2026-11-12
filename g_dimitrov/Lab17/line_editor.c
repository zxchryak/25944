#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#define LINE_LIMIT 40
#define CTRL_D 4
#define CTRL_W 23

static volatile sig_atomic_t interrupted = 0;

static void handle_signal(int sig)
{
    interrupted = sig;
}

static void erase_chars(int count)
{
    while (count-- > 0) {
        fputs("\b \b", stdout);
    }
}

static void append_char(unsigned char line[], int *length, unsigned char ch)
{
    int start;
    int word_length;

    if (*length == LINE_LIMIT) {
        if (ch == ' ') {
            putchar('\n');
            *length = 0;
            return;
        }

        start = *length;
        while (start > 0 && line[start - 1] != ' ') {
            start--;
        }

        if (start > 0) {
            word_length = *length - start;
            erase_chars(word_length);
            putchar('\n');
            memmove(line, line + start, (size_t)word_length);
            fwrite(line, 1, (size_t)word_length, stdout);
            *length = word_length;
        } else {
            /* A word longer than 40 characters must be split. */
            putchar('\n');
            *length = 0;
        }
    }

    line[(*length)++] = ch;
    putchar(ch);
}

int main(void)
{
    struct termios saved;
    struct termios mode;
    struct sigaction action;
    unsigned char line[LINE_LIMIT];
    unsigned char ch;
    int length = 0;
    int old_length;
    int result = EXIT_SUCCESS;
    ssize_t count;

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("tcgetattr");
        return EXIT_FAILURE;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1 ||
        sigaction(SIGHUP, &action, NULL) == -1 ||
        sigaction(SIGQUIT, &action, NULL) == -1 ||
        sigaction(SIGPIPE, &action, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    mode = saved;
    mode.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    mode.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | IXOFF | ISTRIP);
    mode.c_oflag |= OPOST | ONLCR;
    mode.c_cc[VMIN] = 1;
    mode.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1) {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }

    while (!interrupted) {
        count = read(STDIN_FILENO, &ch, 1);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            result = EXIT_FAILURE;
            break;
        }
        if (count == 0) {
            break;
        }

        if (saved.c_cc[VERASE] != (cc_t)_POSIX_VDISABLE &&
            ch == saved.c_cc[VERASE]) {
            if (length > 0) {
                erase_chars(1);
                length--;
            }
        } else if (saved.c_cc[VKILL] != (cc_t)_POSIX_VDISABLE &&
                   ch == saved.c_cc[VKILL]) {
            erase_chars(length);
            length = 0;
        } else if (ch == CTRL_W) {
            old_length = length;
            while (length > 0 && line[length - 1] == ' ') {
                length--;
            }
            while (length > 0 && line[length - 1] != ' ') {
                length--;
            }
            erase_chars(old_length - length);
        } else if (ch == CTRL_D) {
            if (length == 0) {
                break;
            }
            putchar('\a');
        } else if (isprint(ch)) {
            append_char(line, &length, ch);
        } else {
            putchar('\a');
        }

        if (fflush(stdout) == EOF) {
            result = EXIT_FAILURE;
            break;
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1) {
        perror("tcsetattr restore");
        return EXIT_FAILURE;
    }

    if (interrupted) {
        return EXIT_FAILURE;
    }
    return result;
}
