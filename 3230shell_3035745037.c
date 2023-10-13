//Student Name: Li Hoi Kit
//Student No.: 3035745037
//Development Platform: Windows WSL Ubuntu 
//Completion state: Everything not in the bonus part + background process command &. SIGCHLD is not done.


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <errno.h>
#include <signal.h>

//Checks if input is not started/ended with "|".
int validPipe_Start_End(char input[], int string_length){
    //Detect if the input starts with a "|", ignoring space
    for(int i = 0; i < string_length; i++){
        if(input[i] != '|' && input[i] != ' ')
            break; //Valid
        if(input[i] == '|')
            return 0; //Invalid!
    }

    //Detect if the input ends with a "|", ignoring space
    for(int i = string_length - 1; i >= 0; i--){
        if(input[i] != '|' && input[i] != ' ')
            break; //Valid
        if(input[i] == '|')
            return 0; //Invalid!
    }

    return 1; //Empty string
}

//Checks if input has two consecutive "|".
int validPipe_consecutivePipe(char input[], int string_length){
    int indexOfFirstPipe = -1;
    for(int i = 0; i < string_length; i++){
        if(input[i] == '|'){
            if(indexOfFirstPipe == -1)
                indexOfFirstPipe = i;
            else
                return 0; //consecutive pipe!
        }
        else if(input[i] != ' '){ // is a character that is not space and not |
            indexOfFirstPipe = -1; //reset
        }
    }

    return 1; //Valid
}

//Checks if input is a background process (valid input with '&' at the end). Returns 1 if valid, returns 0 if no '&' in the input, returns -1 if '&' is invalid.
int isBackground(char input[]){
    char* symbolPos = strchr(input, '&');
    if(symbolPos == NULL) //No '&' in string
        return 0;
    else{
        symbolPos++;
        while(*symbolPos != '\0'){
            if(*symbolPos != ' ')
                return -1; //invalid to have a non-space character after &
            symbolPos++;
        }
        return 1; //valid as everything after '&' is space
    }
}

//Prints shell message and gets input from the user. Changes the arguments array and returns the number of arguments. Divide arguments into different programs. If return -1, the input starts/ends with "|". If return -2, the input has two consecutive "|".
int getInput(char* arguments[], int* programCount, int programArgumentIndex[], int* isBackgroundCommand){
    char input[1025]; //char array for fgets. 1024 character + termination character \0
    char* programArguments[6];
    int argumentCount = 0;
    *programCount = 0;
    
    //Initialize the arguments to null, so that exec function knows where the arguments end. Extra 5 elements to indicate null
    for(int i = 0; i < 34; i++){
        arguments[i] = NULL;
    }

    //Get input from the user
    printf("$$ 3230shell ## ");
    fgets(input, 1025, stdin);

    input[strcspn(input,"\n")] = 0;//Remove newline character at the end of string
    int input_length = strlen(input);

    //Check pipe errors
    if(validPipe_Start_End(input, input_length) == 0) return -1;
    if(validPipe_consecutivePipe(input, input_length) == 0) return -2;

    //& command
    *isBackgroundCommand = isBackground(input);
    if(*isBackgroundCommand == 1){
        char* symbolPos = strchr(input, '&');
        *symbolPos = '\0';
    }

    //Break input into separate programs
    programArguments[*programCount] = strtok(input, "|");
    while(programArguments[*programCount] != NULL && *programCount < 5){
        (*programCount)++;
        programArguments[*programCount] = strtok(NULL , "|");
    }

    //Break program arguments into separate arguments
    for(int i = 0; i < *programCount; i++){
        programArgumentIndex[i] = argumentCount;
        arguments[argumentCount] = strtok(programArguments[i]," ");
        while(arguments[argumentCount] != NULL && argumentCount < 30){
            argumentCount++;
            arguments[argumentCount] = strtok(NULL , " ");
        }
        argumentCount++;
    }

    return argumentCount - *programCount; //subtract the null elements in arguments array to get the correct number of arguments
}

//Returns the type of path. Returns 0 if path is in the environment variables or if the input length makes no sense, returns 1 otherwise (absolute path/relative path)
int pathType(char* path){
    if(strlen(path) == 0) return 0; //Makes no sense
    if(path[0] == '/' || path[0] == '.') return 1; //Absolute path
    return 0; //Environment variables
}

//Run the programs
void runPrograms(char* arguments[], int programArgumentIndex[], int programCount, int isTimeX, int isBackgroundCommand){

     //Add a signal mask before forking to prevent the SIGUSR1 of parent from being handled before child gets to sigwait for the signal
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    int childPid[5]; //for parent to track the pid of children
    int pipeDes[4][2]; //pipe for each two commands

    //Create pipes
    for(int i = 0; i < programCount - 1; i++){
        if(pipe(pipeDes[i]) == -1){ 
            printf("3230Shell: %s\n", strerror(errno)); //Error! terminate the process
            exit(1);
        }
    }

    //Create child
    for(int i = 0; i < programCount; i++){
        childPid[i] = fork();
        printf("%dth child [%d] forked\n", i, getpid());
        if(childPid[i] < 0){
            printf("%s\n", "Process creation failed!");
            return;
        } else if(childPid[i] == 0){ //child
            if(i != 0){  //only redirect prev output to input if it is not the first command
                dup2(pipeDes[i-1][0],STDIN_FILENO);
            }
            else{
                if(isBackgroundCommand == 1) //close stdin of first command if it is background command
                    close(STDIN_FILENO);
            }

            // printf("%d\n", programCount);
            if (i != programCount - 1) // only redirect output to pipe if it is not the last command
                dup2(pipeDes[i][1],STDOUT_FILENO);

            //close these unnecessary pipes
            for(int i = 0; i < programCount - 1; i++){
                close(pipeDes[i][0]);
                close(pipeDes[i][1]);
            }

            //has to wait for previous commands to finish
            int sig;
            sigwait(&mask, &sig);
            printf("%s\n", arguments[programArgumentIndex[i]]);
            if(pathType(arguments[programArgumentIndex[i]])){ //absolute/relative path

                execv(arguments[programArgumentIndex[i]], &arguments[programArgumentIndex[i]]);
            }else{ //environment variables
                execvp(arguments[programArgumentIndex[i]], &arguments[programArgumentIndex[i]]);
            }

            //Error! terminate the process
            printf("3230Shell: %s\n", strerror(errno));
            exit(1);
        }
    }

    //close these unnecessary pipes
    for(int i = 0; i < programCount - 1; i++){
        close(pipeDes[i][0]);
        close(pipeDes[i][1]);
    }
    
    //Store the time used by child from wait system call
    float usertime[5];
    float systime[5];

    //run the command one by one
    for(int i = 0; i < programCount; i++){
        struct rusage checkTime;
        int childStatus;
        kill(childPid[i], SIGUSR1);//ready to go. Send SIGUSR1 to child.
        if(isBackgroundCommand == 0) //blocks the shell if and only if it is a foreground command
            wait4(childPid[i] ,&childStatus, 0, &checkTime); //retrieve info

        usertime[i] = (float) checkTime.ru_utime.tv_sec + checkTime.ru_utime.tv_usec/1000000.0;
        systime[i] = (float) checkTime.ru_stime.tv_sec + checkTime.ru_stime.tv_usec/1000000.0;

        if(WIFSIGNALED(childStatus)){ //Child terminated by a signal
            printf("%s\n", strsignal(WTERMSIG(childStatus)));
        }
    }

    if(isTimeX == 1){ //print information for timeX command
        for(int i = 0; i < programCount; i++){
            if(pathType(arguments[programArgumentIndex[i]]) == 1){ //absolute/relative path
                int argProgramNameStartIndex;
                for(int j = strlen(arguments[programArgumentIndex[i]]) - 1; j >= 0; j--){ //get the program name
                    if(arguments[programArgumentIndex[i]][j] == '/'){
                        argProgramNameStartIndex = j + 1;
                        break;
                    }
                }

                printf("(PID)%d  (CMD)%s    (user)%0.3f s  (sys)%0.3f s\n",childPid[i] ,&arguments[programArgumentIndex[i]][argProgramNameStartIndex] , usertime[i], systime[i]);
            }
            else{
                printf("(PID)%d  (CMD)%s    (user)%0.3f s  (sys)%0.3f s\n",childPid[i] ,arguments[programArgumentIndex[i]] , usertime[i], systime[i]);
            }
        }
    }
}

//Checks if the command is an exit command. Return 0 if it is not, return 1 if it is, return 2 if it is an invalid exit command
int isExit(int argumentCount, char* arguments[]){
    if(strcmp(arguments[0],"exit") == 0){ //If the first argument is exit
        if(argumentCount == 1) 
            return 1; //Correct exit command
        else 
            return 2; //With arguments. Incorrect exit command
    }
    else
        return 0; //Not an exit command
}

//Checks if the command is an timeX command. Return 0 if it is not, return 1 if it is, return 2 if it is an invalid timeX command
int isTimeX(int argumentCount, char* arguments[]){
    if(strcmp(arguments[0],"timeX") == 0){ //If the first argument is timeX
        if(argumentCount == 1) 
            return 2; //Incorrect exit command
        else 
            return 1; //With other arguments, valid timeX usage.
    }
    else
        return 0; //Not an TimeX command
}

//Signal handler for SIGINT
void sigint_handler(int signo){
    //print prompt message
    printf("\n$$ 3230shell ## ");
    fflush(stdout);
}

void sigusr1_handler(int signo){
    //do nothing
}

int main(){
    //Set up signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, sigusr1_handler);

    //Initialize variables for input
    int argumentCount;
    char* arguments[35]; //30 arguments + 4 NULL pointer at the end of each program
    arguments[34] = NULL; //this shall always be NULL
    int programCount;
    int programArgumentIndex[5]; //Indicates which argument denotes the start of the program
    int isBackgroundCommand;
    
    //Main loop body
    while(1){

        //Get input from user
        argumentCount = getInput(arguments, &programCount, programArgumentIndex, &isBackgroundCommand);
        if(argumentCount == 0) continue; //no input?
        if(argumentCount == -1){
            printf("3230shell: should not have | at the start or end of command\n");
            continue;
        }
        if(argumentCount == -2){
            printf("3230shell: should not have two consecutive | in-between command\n");
            continue;
        }
        if(isBackgroundCommand == -1){
            printf("3230shell: '&' should not appear in the middle of the command line\n");
            continue;
        }

        //Exit command behavior
        int isExitCommand = isExit(argumentCount, arguments); 
        if(isExitCommand == 1){
            printf("3230shell: Terminated\n");
            exit(0);
        }
        else if(isExitCommand == 2){
            printf("3230shell: \"exit\" with other arguments!!!\n");
            continue;
        }

        //timeX command behavior
        int isTimeXCommand = isTimeX(argumentCount, arguments);
        if(isTimeXCommand == 1){
            programArgumentIndex[0]++; //first program starts at second argument
            if(isBackgroundCommand == 1){
                printf("3230shell: \"timeX\" cannot be run in background mode\n");
                continue;
            }
        }
        if(isTimeXCommand == 2){
            printf("3230shell: \"timeX\" cannot be a standalone command\n");
            continue;
        }

        runPrograms(arguments, programArgumentIndex, programCount, isTimeXCommand, isBackgroundCommand);
    }

    return 0;
}