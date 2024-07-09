#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>

void main(){
    char input;
    int status;

    int pid = fork();

    if(pid == 0){
        printf("\n Enter any one : A , B, C: ");
        scanf("%c",&input);


        if (input == 'A'){
            printf("Process Id:%d, Parent  Process Id:%d\n", getpid(), getppid());
            exit(1);
        }

        else if(input == 'B'){
            printf("Process Id:%d, Parent  Process Id:%d\n", getpid(), getppid());
        }

        else if(input == 'C'){

            for(int i =0;i<5;i++){
                 sleep(1);
                printf("Process Id:%d, Parent  Process Id:%d\n", getpid(), getppid());
            }
            int num  = 10/0;
            
        }
    }

   else if (pid > 0) { // Parent process
           {
        int k=wait(&status); 

        printf("\nValue of K is : %d",k);

        if (WIFEXITED(status)) 
        {
            printf("\nNormal Exit: %d \n", WEXITSTATUS(status));
        } 
        if(WIFSIGNALED(status))
        {
            printf("Signalled Exit: %d\n", WTERMSIG(status));
        }
    }
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE); // Exit if fork fails
    }

    
}