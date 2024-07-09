#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

//status variable to store the status of the current program
int status;

int main(){
//this will create child 1 (C1) 
    int p = fork();
//this can be executed by both C1 to create grand child or parent can create a child 2 (C2) 
    int p1 = fork();

//p will be 0 for child 1 only
    if(p==0){
        //p1 and p both will be 0 only for grand child only
        if(p == 0 && p1==0 ){

    printf("\nGC will now differenciate");
//buffer to store the value of the directory
        char s[100];
               //printing current working directory
    printf("\nThe current working directory of the child process is %s\n", getcwd(s, 100));
  
    // using the command chdir 
    chdir("/Users/rohansethi/Downloads/ASP_Programs/test/chapter5/GC");
  
    // printing current working directory
    printf("The new working directory of the child process is %s\n", getcwd(s, 100));
//using ls-1
    char *programName = "ls";
    char *args[] = {programName, "-1", "/Users/rohansethi/Downloads/ASP_Programs/test/chapter5/GC", NULL};
 
    execvp(programName, args);

        
        exit(0);

        }
        else{
            //C1 process now C1 will wait for GC
            printf("\n C1 waiting for GC ");
            //getting the pid of grandchild and passing it to waitpid
        pid_t waited_pid = waitpid(p1, &status, 0);
         
        if (waited_pid == p1) {
            printf("\nChild 1 confirmed: grand child (PID: %d) finished.\n", p1);
            //for normal exit
            if (WIFEXITED(status)) 
            {
                printf("\nNormal Exit: %d \n", WEXITSTATUS(status));
            } 
            // for abnormal exit
            if(WIFSIGNALED(status))
            {
                printf("Signalled Exit: %d\n", WTERMSIG(status));
            }
        }
            }                               

    }


    else if(p>0 && p1>0){
        //Parent process
            printf("\nWe are in the parent process : Parent process terminated !!\n");
        
        exit(0);
    }
    return 0;
    
}