#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

void executionStat(pid_t pid){
  // File address: /proc/[pid]/stat or /proc/[pid]/status
  // exit_status: 0 for EXCODE, 1 for EXSIG
  // printf("endpoint_1\n");
  int PID = pid;
  char path_stat[256];
  char path_status[256];
  sprintf(path_stat, "/proc/%d/stat", PID);
  sprintf(path_status, "/proc/%d/status", PID);
  // printf("stat: %s, status: %s", path_stat, path_status);
  FILE *fp_stat = fopen(path_stat, "r");
  FILE *fp_status = fopen(path_status, "r");
  
  if (fp_stat == NULL || fp_status == NULL){
    printf("JCshell: %s\n", strerror(errno));
    return;
  }

  char *CMD = NULL; // index 1 in stat
  char STATE; // index 2 in stat
  int EXCODE; // index 51 in stat
  char * EXSIG; // obtained by strsignal(EXCODE)
  int PPID; // index 3 in stat
  long unsigned USER_TIME;  // user mode time, index 13 in stat
  long unsigned SYS_TIME; // kernel mode time, index 14 in stat
  unsigned long int VCTX; // voluntary context switches
  unsigned long int NVCTX; // involuntary context switches


  char *line = NULL;
  size_t size = 0;
  if (getline(&line, &size, fp_stat) != -1){
    // printf("%s\n", line);
    char *token = strtok(line, " ");
    
    int index = 0;
    while (token){
      // if (index == 1){
      //   CMD = token;
      // }
      if (index == 2){
        STATE = token[0];
      }
      else if (index == 51){
        EXCODE = atoi(token);
        EXSIG = strsignal(EXCODE);
      }
      else if (index == 3){
        PPID = atoi(token);
      }
      else if (index == 13){
        USER_TIME = strtol(token, NULL, 10);
      }
      else if (index == 14){
        SYS_TIME = strtol(token, NULL, 10);
      }
      token = strtok(NULL, " ");
      // printf("%s\n", token);
      index++;
      
      // printf("endpoint_1\n");
    }
    line = NULL;
  }


  char *line_dup = NULL;
  CMD = (char *)malloc(sizeof(char) * 256);
  while (getline(&line_dup, &size, fp_status) != -1){
    char * vctx = "voluntary_ctxt_switches:";
    char * nvctx = "nonvoluntary_ctxt_switches:";
    char * name = "Name:";
    if (strncmp(line_dup, name, strlen(name)) == 0){
      char *token_dup = strtok(line_dup, "\t");
      int index = 0;
      while (token_dup){
        // printf("%d\n", index);
        if (index == 1){
          int cnt = 0;
          while (token_dup[cnt] != '\n'){
            CMD[cnt] = token_dup[cnt];
            cnt++;
          }
          CMD[cnt] = '\0';
          break;
        }
        token_dup = strtok(NULL, " ");
        index++;
      }
    }
    if (strncmp(line_dup, vctx, strlen(vctx)) == 0)
    {
      char *token = strtok(line_dup, "\t");
      int index = 0;
      while (token){
        if (index == 1){
          VCTX = strtol(token, NULL, 10);
        }
        token = strtok(NULL, " ");
        index++;
      }
    }
    if (strncmp(line_dup, nvctx, strlen(nvctx)) == 0){
      char *token = strtok(line_dup, "\t");
      int index = 0;
      while (token){
        if (index == 1){
          NVCTX = strtol(token, NULL, 10);
        }
        token = strtok(NULL, " ");
        index++;
      }
    }
  }

  printf("(PID)%d (CMD)%s (STATE)%c (EXCODE)%d (EXSIG)%s (PPID)%d (USER)%lu (SYS)%lu (VCTX)%lu (NVCTX)%lu\n", PID, CMD, STATE, EXCODE, EXSIG, PPID, USER_TIME, SYS_TIME, VCTX, NVCTX);
  // 
}


int main() {
  pid_t pid;

  pid = fork();

  if (pid < 0) { // fork error
    printf("fork: error no = %s\n", strerror(errno));
    exit(-1);
  } else if (pid == 0) { // child process
    printf("child: I am a child process, with pid %d\n", (int)getpid());
    return 0;
  } else { // parent process
    // siginfo_t is only a placeholder here, can also use siginto.si_pid to access pid
    siginfo_t info;
    int status;

    // wait for child to terminate and kept as zombie process
    // 1st param: P_ALL := any child process; P_PID := process specified as 2nd param
    // WNOWAIT: Leave the child in a waitable state;
    //    so that later another wait call can be used to again retrieve the child status information.
    // WEXITED: wait for processes that have exited
    int ret = waitid(P_ALL, 0, &info, WNOWAIT | WEXITED);
    if (!ret) {
      printf("Child process %d has been turned into zombine status\n", (int) info.si_pid);
      executionStat(info.si_pid);
      waitpid(info.si_pid, &status, 0);
      printf("Child process %d's resource has been clean\n", (int) info.si_pid);
    } else {
      perror("waitid");
    }

    return 0;
  }
}