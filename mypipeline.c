#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/wait.h>
int main(int argc, char *argv[]){
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
    if(pid1==0){
      
        close(STDOUT_FILENO);   
        int newfd1 = dup(originalPipe_fd[1]);

        if (newfd1 == -1) {
            perror("dup");
            exit(1);
        }

        close(originalPipe_fd[1]);
        execlp("ls", "ls", "-lsa", (char *)NULL);
        perror("execlp");
        exit(1);
    }

    else{
        close(originalPipe_fd[1]);
    }



    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(1);
    }

    if(pid2 == 0){
        //child 2 code:
        close(STDIN_FILENO);
        int newfd2 = dup(originalPipe_fd[0]);

        if (newfd2 == -1) {
            perror("dup");
            exit(1);
        }

        close(originalPipe_fd[0]);
        execlp("tail", "tail", "-n", "3", (char *)NULL);
        perror("execlp");
        exit(1);

    }

    else{
        close(originalPipe_fd[0]);
    }
    int status;
    waitpid(pid1, &status, 0);  
    waitpid(pid2, &status, 0);  

    return 0;
}