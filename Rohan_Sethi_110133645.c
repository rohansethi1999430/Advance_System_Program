#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int *array_ptr = NULL; // Global pointer to shared memory



void handler(int signo) 
{
//stop the chil process
    kill(array_ptr[1], SIGSTOP);
    kill(array_ptr[2], SIGSTOP);

//change the process group id using the setpgid() system call
    setpgid(array_ptr[0], array_ptr[1]);
    setpgid(array_ptr[0], array_ptr[2]);

// Check if array_ptr[3] is 1, if so, exit
    if (array_ptr[3] == 1) 
    {
        exit(EXIT_SUCCESS);
    }

// set the flag to 1
    array_ptr[3] = 1;
}
int flag = 1;

int main(int argc, char *argv[]) 
{
    int shmid;

    // Create shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, 5 * sizeof(int), IPC_CREAT | 0666)) < 0 && flag == 1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment to array_ptr
    if ((array_ptr = shmat(shmid, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialize array value to 0
    array_ptr[4] = 0;
    array_ptr[3] = 0;
    
    //pid of main process
    array_ptr[0] = getpid(); 


//calling 2 fork
    int pid10 = fork();
    int pid20 = fork();

    if (pid10 == 0 && pid20 == 0 && flag == 1)
    {
        //do nothing
    }
    else
    {
        // Parent process
        setpgid(0, getpid());

        if (pid10 > 0 && pid20 > 0)
        {
            //pid of 1st child
            array_ptr[1] = pid10; 
            //pid of 2nd child
            array_ptr[2] = pid20; 
        }

        //registering the signal
        signal(SIGINT, handler);
    }

   // execute(); // Execute the main logic of the program

        do 
    {
        sleep(2);
//checking if the process is the main process
        if (array_ptr[3] != 1 && flag==1) 
        {
            if (getpid() != array_ptr[1]) {
                printf("From process Main\n");
            } else if (getpid() == array_ptr[1]) {
                printf("From process C1\n");
            } else if (getpid() == array_ptr[2]) {
                printf("From process C2\n");
            } else {
                printf("From process GC\n");
            }
        }
        //else means the main processs is terminated and now child process are only running 
        else
        {
            //check if the process is the C2 process 
            if (array_ptr[4] !=0 && array_ptr[4]!=2 && array_ptr[3]==1)
            {
                printf("\nThis is from C2 process (%d)\n", getpid());
                array_ptr[4] = 2;
                //stopping the C2 process and now the GC process will continue
                kill(array_ptr[2], SIGCONT);
                kill(array_ptr[1], SIGSTOP);
            }
            else if (array_ptr[4] !=0 && array_ptr[4]!=1 && array_ptr[3]==1) 
            {
                printf("This is from GC process (%d)\n", getpid());
                array_ptr[4] = 0;
                //stopping the GC process after printing and now C1 process will continue
                kill(array_ptr[0], SIGCONT);
                kill(array_ptr[2], SIGSTOP);
            }
                if (array_ptr[4] !=1 && array_ptr[4]!=2 && array_ptr[3]==1) 
            {
                printf("This is from C1 (%d)\n", getpid());
                array_ptr[4] = 1;
                //now sending the stop signal to the C1 process and continue signal C2 process
                kill(array_ptr[1], SIGCONT);
                kill(array_ptr[0], SIGSTOP);
            }
        }
    } while (1);

    return 0;
}
