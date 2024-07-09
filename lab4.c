#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    int p,p1;
    p = fork();
    p1 = fork();
    int status;
    if(p==-1)
    {
        printf("There is an error while calling fork()");
    }
    if(p>0 && p1==0)
    {   
        printf("\n PID = %d",getpid());
        // for(int i =0;i<3;i++){
        //     sleep(1);
        //     printf("\nWe are in the child 2 process\n");
        // }
       char s[100];
  
   //printing current working directory
    printf("\nThe current working directory of the child process is %s\n", getcwd(s, 100));
  
    // using the command
    chdir("/Users/rohansethi/Downloads/ASP_Programs/test");
  
    // printing current working directory
    printf("The new working directory of the child process is %s\n", getcwd(s, 100));

    char *programName = "ls";
    char *args[] = {programName, "-1", "/Users/rohansethi/Downloads/ASP_Programs/test", NULL};
 
    execvp(programName, args);
  
        exit(0);
    }


    else if(p1> 0 && p > 0)
    { 
    pid_t waited_pid = waitpid(p1, &status, 0);
    if (waited_pid == p1) {
        printf("Parent confirmed: second child (PID: %d) finished.\n", p);
        if (WIFEXITED(status)) 
        {
            printf("\nNormal Exit: %d \n", WEXITSTATUS(status));
        } 
        if(WIFSIGNALED(status))
        {
            printf("Signalled Exit: %d\n", WTERMSIG(status));
        }
    }
        printf("\nWe are in the parent process\n");
       
        
        exit(0);
    }
    return 0;
}

