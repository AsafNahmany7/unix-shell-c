#define HISTLEN 10
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include "LineParser.h"
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>



#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0




typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

char* history[HISTLEN];
int oldest = 0;
int newest = -1;
int historyCount = 0;
process* currentProcesses = NULL;




void freeHistory() {
    for (int i = 0; i < historyCount; i++) {
        int index = (oldest + i) % HISTLEN;
        free(history[index]);
    }
}

void updateHistory(char* cmd) {
    
    newest = (newest + 1) % HISTLEN;
    
    if (historyCount == HISTLEN) {
        free(history[newest]);  
        oldest = (oldest + 1) % HISTLEN;
    } 
    else {
        historyCount++;
    }
    
    history[newest] = strdup(cmd);
}


void printHistory() {
    if (historyCount == 0) {
        printf("No history\n");
        return;
    }
    
    int index = oldest;
    for (int i = 0; i < historyCount; i++) {
        printf("%d  %s", i, history[index]);  
        index = (index + 1) % HISTLEN;
    }
}





void addProcess(process** process_list,cmdLine* cmd ,pid_t pid){
    process* head = *process_list;
    process* newProcess = malloc(sizeof(process));
    newProcess->cmd=cmd;
    newProcess->pid=pid;
    newProcess->status=1;
    newProcess->next=head;
    *process_list = newProcess;

}

void freeProcessList(process* process_list){
    process* current = process_list;
    while(current!=NULL){
        process* toDelete = current;
        current=current->next;
        freeCmdLines(toDelete->cmd);
        free(toDelete);
        
    }
}

void updateProcessStatus(process* process_list,int pid ,int status){
    process* current = process_list;
    int found =0;
    while(current!=NULL&& found==0){
        if(current->pid==pid){
            found = 1;
            current->status=status;  

        }
        current = current->next;
        
    }

}

void updateProcessList(process** process_list){
    process* current = *process_list;
    while (current!=NULL){
        int status;
        int result = waitpid(current->pid,&status,WNOHANG);

        if (result > 0 && WIFEXITED(status)) {
            updateProcessStatus(*process_list,current->pid,-1);
        }

        else if (result > 0 && WIFSIGNALED(status)) {
            updateProcessStatus(*process_list,current->pid,-1);
        }

        else if (result == -1) {
            updateProcessStatus(*process_list,current->pid,-1);
        }

        current=current->next;
    }

}

void printProcessList(process** process_list){
    updateProcessList(process_list);
    process* current1 = *process_list;
    
    printf("PID         Command         STATUS\n");
    while(current1!=NULL){
       
  
        if(current1->status==1){
            printf("%d          %s          RUNNING \n",current1->pid,current1->cmd->arguments[0]);
        }
        if(current1->status==-1){
             printf("%d          %s         TERMINATED \n",current1->pid,current1->cmd->arguments[0]);
        }
        if(current1->status==0){
             printf("%d          %s         SUSPENDED\n",current1->pid,current1->cmd->arguments[0]);
        }
  
        current1=current1->next;
    }

    process* prev = NULL;
    current1 = *process_list;
    
    while(current1 != NULL){
        if(current1->status == -1){
            if(prev == NULL){
                *process_list = current1->next;
                freeCmdLines(current1->cmd);
                free(current1);
                current1 = *process_list;
            }
            else{
                prev->next = current1->next;
                freeCmdLines(current1->cmd);
                free(current1);
                current1 = prev->next;
            }
        }
        else{
            prev = current1;
            current1 = current1->next;
        }
    }
}
    



void execute(cmdLine *pCmdLine){
    execvp(pCmdLine->arguments[0],pCmdLine->arguments);
    perror("execv faild");    
    exit(1);
}


int main(int argc, char *argv[]){

    int debug = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug = 1;
    }

    while(1){
        char cwd[PATH_MAX];

        if (getcwd(cwd,PATH_MAX)!= NULL){
            printf("%s$",cwd);
            fflush(stdout);
        }
        char input[2048];
        if (fgets(input, 2048, stdin) == NULL) {
            exit(0);
        }

    
        if (strcmp(input, "history\n") == 0) {
            printHistory();
            continue;  
        }
    
        if (strcmp(input, "!!\n") == 0) {
            if (historyCount == 0) {
                fprintf(stderr, "No commands in history\n");
                continue;
            }
            
            strcpy(input, history[newest]);
            
        }
    
        if (input[0] == '!' && input[1] != '!' && input[1] != '\n') {
            int n = atoi(input + 1);
            if (n < 0 || n >= historyCount) {
                fprintf(stderr, "Invalid history index\n");
                continue;
            }
            
            int index = (oldest + n) % HISTLEN;
            strcpy(input, history[index]);
            
        }
    
    
        updateHistory(input);
        
        cmdLine *cmd  = parseCmdLines(input);
        
        if(cmd == NULL){
            continue;
        }

        if(strcmp(cmd->arguments[0],"quit") == 0){
            freeCmdLines(cmd);
            freeProcessList(currentProcesses);
            freeHistory();
            exit(0);                 
        }

        if(strcmp(cmd->arguments[0],"cd") == 0){
            if (chdir(cmd->arguments[1]) != 0) {
                perror("cd failed");  
            }
            freeCmdLines(cmd);
            continue;  
        }
        if(strcmp(cmd->arguments[0],"procs") == 0){
            printProcessList(&currentProcesses);  
            freeCmdLines(cmd);
            continue;
        }


        if(strcmp(cmd->arguments[0], "zzzz") == 0){
            int pid = atoi(cmd->arguments[1]);
            if (kill(pid, SIGSTOP) != 0) {
                perror("zzzz failed");
                
            }
            else{
                updateProcessStatus(currentProcesses, pid, 0);            }
            freeCmdLines(cmd);
            continue;
        }

        if(strcmp(cmd->arguments[0], "kuku") == 0){
            int pid = atoi(cmd->arguments[1]);
            if (kill(pid, SIGCONT) != 0) {
                perror("kuku failed");
            }
            else{
                updateProcessStatus(currentProcesses, pid, 1);
            }
            freeCmdLines(cmd);
            continue;
        }

        if(strcmp(cmd->arguments[0], "blast") == 0){
            int pid = atoi(cmd->arguments[1]);
            if (kill(pid, SIGINT) != 0) {
                perror("blast failed");
                
            }
            else{
                updateProcessStatus(currentProcesses, pid, -1);            }
            freeCmdLines(cmd);
            continue;
    }


        //Double command
    if(cmd->next != NULL){
        cmdLine* secondCmd = cmd->next;
        cmd->next = NULL;
    
        if (cmd->outputRedirect != NULL) {
            fprintf(stderr, "Error: output redirect on left side of pipe\n");
            freeCmdLines(cmd);
            freeCmdLines(secondCmd);
            continue;
        }
        if (secondCmd->inputRedirect != NULL) {
            fprintf(stderr, "Error: input redirect on right side of pipe\n");
            freeCmdLines(cmd);
            freeCmdLines(secondCmd);
            continue;
        }
    
        int originalPipe_fd[2];
        if (pipe(originalPipe_fd) == -1) {
            perror("pipe");
            exit(1);
        }
    
        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("fork");
            exit(1);
        }
    
        if(pid1 == 0){
            if (cmd->inputRedirect != NULL) {
                close(STDIN_FILENO);
                open(cmd->inputRedirect, O_RDONLY);
            }
            close(STDOUT_FILENO);   
            int newfd1 = dup(originalPipe_fd[1]);
            if (newfd1 == -1) {
                perror("dup");
                exit(1);
            }
            close(originalPipe_fd[0]);
            close(originalPipe_fd[1]);
            execute(cmd);
        }
        else{
            addProcess(&currentProcesses, cmd, pid1);
            close(originalPipe_fd[1]);
        }
    
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("fork");
            exit(1);
        }
    
        if(pid2 == 0){
            
            close(STDIN_FILENO);
            int newfd2 = dup(originalPipe_fd[0]);
            if (newfd2 == -1) {
                perror("dup");
                exit(1);
            }
            close(originalPipe_fd[0]);
            if(secondCmd->outputRedirect != NULL) {
                close(STDOUT_FILENO);
                open(secondCmd->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            execute(secondCmd);
        }
        else{
            addProcess(&currentProcesses, secondCmd, pid2);
            close(originalPipe_fd[0]);
        }
    
        int status;
        waitpid(pid1, &status, 0);  
        waitpid(pid2, &status, 0);  
    }


        //Single command
        else{ 
            int pid=fork();
            if(pid==0){
                if (cmd->inputRedirect != NULL) {
                    close(0);
                    if (open(cmd->inputRedirect, O_RDONLY) < 0) {
                        perror("open input failed");
                        _exit(1);
                    }   
                }
    
                if (cmd->outputRedirect != NULL) {
                    close(1);
                    if (open(cmd->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644) < 0) {
                        perror("open output failed");
                        _exit(1);
                    }
                }




                if (debug==1){
                    fprintf(stderr, "PID: %d\n", getpid());
                    fprintf(stderr, "Executing command: %s\n", cmd->arguments[0]);
                }
            
                execute(cmd);
            }
            else{
                addProcess(&currentProcesses, cmd, pid);
                if (cmd->blocking == 1) {
                    waitpid(pid, NULL, 0);  
                }
            }
        }
    }

}


