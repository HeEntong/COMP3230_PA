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

// Compared to past year PA, we do not consider any background variables

#define MAX_CMD_LENGTH 1024
#define MAX_CMD_ARGS 30
#define MAX_PIPE_NUM 5

// int jump_flag = 0;
int sigint_sent = 0;
int in_execution = 0;

jmp_buf jmpbuffer;

int max(int a, int b){
  return a > b ? a : b;
}

void space_eliminator(char ** str){
  int lead_ptr = 0, trail_ptr = strlen((*str)) - 1;
  while ((*str)[lead_ptr] == ' '){
    lead_ptr++;
  }
  while ((*str)[trail_ptr] == ' ' || (*str)[trail_ptr] == '\0'){
    trail_ptr--;
  }
  for (int i = lead_ptr; i <= trail_ptr; i++){
    (*str)[i - lead_ptr] = (*str)[i];
  }
  (*str)[trail_ptr - lead_ptr + 1] = '\0';
}

void sigint_handler(int sig_num){
  if (!in_execution){
    fflush(stdout);
    printf("\n## JCshell [%d] ##  ", getpid());
    fflush(stdout);
    // jump_flag = 1;
  }
  else{
    printf("\n");
    sigint_sent = 1;
  }
  return;
}

void sigusr1_handler(int sig_num){
  return;
}

int exit_handler(char **cmds){
  if (strcmp(cmds[0], "exit") == 0){
    if (cmds[1] != NULL){
      printf("JCshell: \"exit\" with other arguments!!!\n");
      return -1;
    }
    else{
      printf("JCShell: Terminated\n");
      exit(0);
    }
  }
  return 0;
}

void statExtractor(pid_t pid){
  
}

int all_spaces(char *arg){
  for (int i = 0; i < strlen(arg); i++){
    if (arg[i] != ' '){
      return 0;
    }
  }
  return 1;
}

int inputClassifier(char *arg){ // 1 for absolute path, 2 for relative path, 3 for directories searching
  if (arg[0] == '/' || arg[0] == '.'){
    return 1;
  }
  return 0;
}

int pipeSplit(char *cmd, char *** splitedcmd, int *pipe_num){
  // pre-process the command
  int ptr = -1;
  cmd[strlen(cmd) - 1] = ' '; // Remove the trailing newline
  for (int i = 0; i < strlen(cmd); i++)
  {
    // Find the valid command segment
    if (cmd[i] != ' ')
    {
      ptr = max(ptr, i);
    }
        }
        cmd[ptr + 1] = '\0';
  /*
  First handle the pipes, detect all illegal consecutive |s and | at the beginning
  */
  for (int i = 0; i < strlen(cmd); i++){
    if (cmd[i] == '|'){
      if (i == 0 || i == strlen(cmd) - 1){
        printf("JCshell: illegal placement of | symbols\n");
        return 0;
      }
      if (cmd[i + 1] == '|'){
        printf("JCshell: should not have two | symbols without in-between command\n");
        return 0;
      }
    }
  }
  char *token = strtok(cmd, "|");
  int cnt = 0;
  char ** dup_cmd_split = (char **)malloc(sizeof(char *) * MAX_PIPE_NUM);
  // token = strtok(NULL, " ");
  while (token){
    dup_cmd_split[cnt] = token;
    token = strtok(NULL, "|");
    cnt++;
  }
  (*splitedcmd) = (char **)malloc(sizeof(char *) * (cnt + 1)); // NULL terminator at the end
  (*splitedcmd)[cnt] = NULL;
  for (int i = 0; i < cnt; i++)
  {
    (*splitedcmd)[i] = dup_cmd_split[i];
    // printf("cmd: %s\n", (*splitedcmd)[i]);
    if (all_spaces((*splitedcmd)[i])){
      printf("JCshell: should not have two | symbols without in-between command\n");
      return 0;
    }
    // printf("cmd: %s\n", splitedcmd[i]);
  }
  free(dup_cmd_split);
  *pipe_num = cnt;
  return 1;
}

void commandSpliter(char *cmd, char ***splitted_cmd_ptr){ // change into an array containing seperate commands
  (*splitted_cmd_ptr) = (char **)malloc(sizeof(char *) * MAX_CMD_ARGS);
  char *token = strtok(cmd, " ");
  int cnt = 0;
  while (token){
    (*splitted_cmd_ptr)[cnt] = token;
    token = strtok(NULL, " ");
    cnt++;
  }
  for (int i = cnt; i < MAX_CMD_ARGS; i++){
    (*splitted_cmd_ptr)[i] = NULL;
  }
}

//, int exit_status, char ** string, char *cmd
void executionStat(pid_t pid, char ** returnString){ // exit_status: 0 for normal (EXCODE), 1 for abnormal (EXSIG)
  // File address: /proc/[pid]/stat or /proc/[pid]/status
  // exit_status: 0 for EXCODE, 1 for EXSIG
  // printf("endpoint_1\n");
  
  char path_stat[256];
  char path_status[256];
  sprintf(path_stat, "/proc/%d/stat", pid);
  sprintf(path_status, "/proc/%d/status", pid);
  // printf("stat: %s, status: %s", path_stat, path_status);
  FILE *fp_stat = fopen(path_stat, "r");
  FILE *fp_status = fopen(path_status, "r");
  
  if (fp_stat == NULL || fp_status == NULL){
    printf("JCshell: %s\n", strerror(errno));
    return;
  }

  char PID[128]; // index 0 in stat
  char CMD[128]; // index 1 in stat
  char STATE[128]; // index 2 in stat
  char EXCODE[128]; // index 51 in stat
  char EXSIG[128]; // obtained by strsignal(EXCODE)
  char PPID[128]; // index 3 in stat
  char USER_TIME[128];  // user mode time, index 13 in stat
  char SYS_TIME[128]; // kernel mode time, index 14 in stat
  char VCTX[128]; // voluntary context switches
  char NVCTX[128]; // involuntary context switches


  char *line = NULL;
  size_t size = 0;
  if (getline(&line, &size, fp_stat) != -1){
    // printf("%s\n", line);
    char *token = strtok(line, " ");
    
    int index = 0;
    while (token){
      if (index == 0){
        strcpy(PID, token);
      }
      if (index == 2){
        strcpy(STATE, token);
      }
      else if (index == 51){
        token[strlen(token) - 1] = '\0';
        strcpy(EXCODE, token);
        strcpy(EXSIG, strsignal(atoi(EXCODE)));
      }
      else if (index == 3){
        strcpy(PPID, token);
      }
      else if (index == 13){
        strcpy(USER_TIME, token);
      }
      else if (index == 14){
        strcpy(SYS_TIME, token);
      }
      token = strtok(NULL, " ");
      // printf("%s\n", token);
      index++;
      
      // printf("endpoint_1\n");
    }
    line = NULL;
  }


  char *line_dup = NULL;
  // CMD = (char *)malloc(sizeof(char) * 256);
  while (getline(&line_dup, &size, fp_status) != -1){
    char * vctx = "voluntary_ctxt_switches:";
    char * nvctx = "nonvoluntary_ctxt_switches:";
    char * name = "Name:";
    if (strncmp(line_dup, name, strlen(name)) == 0){
      char *token = strtok(line_dup, "\t");
      int index = 0;
      while (token){
        // printf("%d\n", index);
        if (index == 1){
          token[strlen(token) - 1] = '\0'; // remove the trailing '\n' in token
          strcpy(CMD, token);
        }
        token = strtok(NULL, "\t");
        index++;
      }
    }
    if (strncmp(line_dup, vctx, strlen(vctx)) == 0)
    {
      char *token = strtok(line_dup, "\t");
      int index = 0;
      while (token){
        if (index == 1){
          token[strlen(token) - 1] = '\0'; // remove the trailing '\n' in token
          strcpy(VCTX, token);
        }
        token = strtok(NULL, "\t");
        index++;
      }
    }
    if (strncmp(line_dup, nvctx, strlen(nvctx)) == 0){
      char *token = strtok(line_dup, "\t");
      int index = 0;
      while (token){
        if (index == 1){
          token[strlen(token) - 1] = '\0'; // remove the trailing '\n' in token
          strcpy(NVCTX, token);
        }
        token = strtok(NULL, "\t");
        index++;
      }
    }
  }
  
  strcpy(returnString[0], PID);
  strcpy(returnString[1], CMD);
  strcpy(returnString[2], STATE);
  strcpy(returnString[3], EXCODE);
  strcpy(returnString[4], EXSIG);
  strcpy(returnString[5], PPID);
  strcpy(returnString[6], USER_TIME);
  strcpy(returnString[7], SYS_TIME);
  strcpy(returnString[8], VCTX);
  strcpy(returnString[9], NVCTX);

  fclose(fp_stat);
  fclose(fp_status);

}

void commandExecutor(char **splitedcmd, int pipe_num){
  char **cmds[pipe_num + 1];
  for (int i = 0; i < pipe_num; i++){ // Each pipe execution corresponds to one child process (the code following will otherwise halt the parent process)
    char **splitted_cmd = NULL;
    char ***splitted_cmd_ptr = &splitted_cmd;
    commandSpliter(splitedcmd[i], splitted_cmd_ptr); /* split the command in each pipe */
    if (exit_handler(splitted_cmd) < 0){
      return;
    }

    cmds[i] = splitted_cmd; // length of cmds shall not exceed MAX_PIPE_NUM
  }
  cmds[pipe_num] = NULL; // NULL terminator


    int child_process_pid[MAX_PIPE_NUM]; // Tracking the pid of child processes
    int pipefd[MAX_PIPE_NUM - 1][2];
    for (int i = 0; i < pipe_num - 1; i++){
      if (pipe(pipefd[i]) < 0){
        printf("JCshell: %s\n", strerror(errno)); // Pipe creation fails
        // exit(1);
        return;
      }
  }


  for (int i = 0; i < pipe_num; i++){ // execute the processes sequentially
    child_process_pid[i] = fork();
    // printf("%d: %d\n", i, child_process_pid[i]);
    // printf("%dth child [%d] forked\n", i, getpid());
    if (child_process_pid[i] == -1)
    {
      printf("JCshell: %s\n", strerror(errno));
      return;
    }
    else if (child_process_pid[i] == 0){  // Child process?
      if (i > 0)
      {
        // close(pipefd[i - 1][1]); // close the write end of the previous pipe
        dup2(pipefd[i - 1][0], STDIN_FILENO); // redirect the stdin to the read end of the previous pipe
        // close(pipefd[i - 1][0]); // close the read end of the previous pipe
      }
      // printf("%d\n", i);
      if (i < pipe_num - 1)
      {
        // close(pipefd[i][0]);
        dup2(pipefd[i][1], STDOUT_FILENO);
        // close(pipefd[i][1]);
      }

      for (int j = 0; j < pipe_num - 1; j++){
        close(pipefd[j][0]);
        close(pipefd[j][1]);
      }


      if (inputClassifier(cmds[i][0]) == 0)
      {
        execvp(cmds[i][0], &cmds[i][0]);
      }
      else{
        execv(cmds[i][0], &cmds[i][0]);
      }


      printf("JCshell: '%s': %s\n", cmds[i][0], strerror(errno));
      exit(1);
    }
    // else{
    //   // printf("child [%d] waiting....\n", i);
    //   wait(NULL);
    // }


  }

  for (int j = 0; j < pipe_num - 1; j++){
    close(pipefd[j][0]);
    close(pipefd[j][1]);
  }

  siginfo_t info;
  int status;
  int exit_status[pipe_num];
  int cnt = 0;

  int excodes[pipe_num];
  char * exsigs[pipe_num];
  for (int i = 0; i < pipe_num; i++){
    exsigs[i] = (char *)malloc(sizeof(char) * 64);
  }
  for (int i = 0; i < pipe_num; i++){
    excodes[i] = -1;
  }

  char **output_strings[pipe_num];
  for (int i = 0; i < pipe_num; i++){
    output_strings[i] = (char **)malloc(sizeof(char *) * 10);
    for (int j = 0; j < 10; j++) {
        output_strings[i][j] = (char *)malloc(sizeof(char) * 128);
    }
  }

  // int ret_val = waitid(P_ALL, 0, &info, WNOWAIT | WEXITED);
  while (1){
      pid_t i = waitid(P_ALL, 0, &info, WNOWAIT | WEXITED);
    if (!i){
      executionStat(info.si_pid, output_strings[cnt]);
      waitpid(info.si_pid, &status, 0);
      if (WIFEXITED(status)){
        exit_status[cnt] = 0;
        excodes[cnt] = WEXITSTATUS(status);
      }
      else if (WIFSIGNALED(status)){
        exit_status[cnt] = 1;
        strcpy(exsigs[cnt], strsignal(WTERMSIG(status)));
      }
      // if (!exit_status){
      //   printf("\r(PID)%s (CMD)%s (STATE)%s (EXCODE)%d (PPID)%s (USER)%s (SYS)%s (VCTX)%s (NVCTX)%s\n", childStat[0], childStat[1], childStat[2], WEXITSTATUS(status), childStat[5], childStat[6], childStat[7], childStat[8], childStat[9]);
      // }
      // else{
      //   printf("\r(PID)%s (CMD)%s (STATE)%s (EXSIG)%s (PPID)%s (USER)%s (SYS)%s (VCTX)%s (NVCTX)%s\n", childStat[0], childStat[1], childStat[2], strsignal(WTERMSIG(status)), childStat[5], childStat[6], childStat[7], childStat[8], childStat[9]);
      // }
      cnt++;
      if (cnt == pipe_num)
        break;
    }
    }

    for (int i = 0; i < pipe_num; i++){
      if (exit_status[i] == 0){
        printf("\r(PID)%s (CMD)%s (STATE)%s (EXCODE)%d (PPID)%s (USER)%s (SYS)%s (VCTX)%s (NVCTX)%s\n", output_strings[i][0], output_strings[i][1], output_strings[i][2], excodes[i], output_strings[i][5], output_strings[i][6], output_strings[i][7], output_strings[i][8], output_strings[i][9]);
      }
      else{
        printf("\r(PID)%s (CMD)%s (STATE)%s (EXSIG)%s (PPID)%s (USER)%s (SYS)%s (VCTX)%s (NVCTX)%s\n", output_strings[i][0], output_strings[i][1], output_strings[i][2], exsigs[i], output_strings[i][5], output_strings[i][6], output_strings[i][7], output_strings[i][8], output_strings[i][9]);
      }
    }
}

int main(){
    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, sigusr1_handler);
    char command[MAX_CMD_LENGTH + 1]; // NULL temrinator at the end


    while (1){
      if (!sigint_sent){
        printf("## JCshell [%d] ##  ", getpid());
      }
      // Interrupt shall be handled immediately
      if (fgets(command, MAX_CMD_LENGTH, stdin) == NULL){
        // Ensure that no infinite loop will occur when EOF is reached
        exit(0);
      }
      char **splitedcmd = NULL;
      char ***splitedcmd_ptr = &splitedcmd;
      int pipe_num = 0;
      int *pip_num_ptr = &pipe_num;
      int handle_status = pipeSplit(command, splitedcmd_ptr, pip_num_ptr);
      if (!handle_status){
        continue;
      }
      // printf("%d", splitedcmd[0] == NULL);
      for (int i = 0; i < pipe_num; i++){
        char **ptr = &splitedcmd[i];
        space_eliminator(ptr);
      }
      if (pipe_num > MAX_PIPE_NUM){
        printf("JCshell: too many pipes\n");
        continue;
      }
      // for (int i = 0; i < pipe_num; i++){
      //   printf("%s\n", splitedcmd[i]);
      // }
      in_execution = 1;
      commandExecutor(splitedcmd, pipe_num);
      in_execution = 0;
      if (sigint_sent)
      {
        fflush(stdout);
        printf("## JCshell [%d] ##  ", getpid());
        continue;
      }
    }
}