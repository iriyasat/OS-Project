#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Please provide a positive integer as an argument.\n");
        fprintf(stderr, "Usage: %s <positive_integer>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Error: Please provide a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Error: fork() failed\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("%d", n);
        
        while (n != 1) {
            if (n % 2 == 0) {
                n = n / 2;
            } else {
                n = 3 * n + 1;
            }
            printf(", %d", n);
        }
        printf("\n");
        
        exit(EXIT_SUCCESS);
    } else {
        int status;
        wait(&status);
        
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != EXIT_SUCCESS) {
                fprintf(stderr, "Error: Child process exited with error status.\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Error: Child process terminated abnormally.\n");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
