#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
void getProcessInfo(pid_t pid) {
    char statPath[256];
    sprintf(statPath, "/proc/%d/stat", pid);

    FILE* statFile = fopen(statPath, "r");
    if (statFile == NULL) {
        printf("Failed to open stat file for PID %d\n", pid);
        return;
    }

    char statLine[1024];
    fgets(statLine, 1024, statFile);

    char* token = strtok(statLine, " ");
    int i = 0;
    while (token != NULL) {
        switch (i) {
            case 0: printf("PID: %s\n", token); break;
            case 1: printf("CMD: %s\n", token); break;
            case 2: printf("STATE: %s\n", token); break;
            case 3: printf("EXCODE: %s\n", token); break;
            case 4: printf("EXSIG: %s\n", token); break;
            case 5: printf("PPID: %s\n", token); break;
            case 13: printf("USER: %s\n", token); break;
            case 14: printf("SYS: %s\n", token); break;
            case 23: printf("VCTX: %s\n", token); break;
            case 24: printf("NVCTX: %s\n", token); break;
        }

        token = strtok(NULL, " ");
        i++;
    }

    fclose(statFile);
}

int main() {
    pid_t pid = getpid(); // Replace with the actual PID you want to retrieve information for
    getProcessInfo(pid);

    return 0;
}