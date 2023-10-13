#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_CHILDREN 5

int main() {
    pid_t child_pids[NUM_CHILDREN];
    int status;
    int i;

    // Fork child processes
    for (i = 0; i < NUM_CHILDREN; i++) {
        child_pids[i] = fork();

        if (child_pids[i] == 0) {
            // Child process code here
            // ...
            sleep(10 - i);  // Sleep for different durations to simulate different execution times
            printf("Child process with PID %d has finished.\n", getpid());
            exit(0);  // Exit child process
        }
    }
    int cnt = 0;
    // Parent process code
    while (1) {
        pid_t terminated_pid = waitpid(-1, &status, WNOHANG);

        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Child process with PID %d has terminated normally.\n", terminated_pid);
            } else if (WIFSIGNALED(status)) {
                printf("Child process with PID %d has terminated due to a signal.\n", terminated_pid);
            } else if (WIFSTOPPED(status)) {
                printf("Child process with PID %d has stopped.\n", terminated_pid);
            }

            cnt++;
            if (cnt == NUM_CHILDREN) break;
             // Break the loop after any child process terminates
        }
    }

    // Continue with the rest of the parent process code
    // ...

    return 0;
}