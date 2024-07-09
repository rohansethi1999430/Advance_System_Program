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
    // Stop child processes
    kill(array_ptr[1], SIGSTOP);
    kill(array_ptr[2], SIGSTOP);

    // Set process group IDs for child processes
    setpgid(array_ptr[0], array_ptr[1]);
    setpgid(array_ptr[0], array_ptr[2]);

    // Check if array_ptr[3] is 1, if so, exit
    if (array_ptr[3] == 1) 
    {
        exit(EXIT_SUCCESS);
    }

    // Set array_ptr[3] to 1 to indicate the flag
    array_ptr[3] = 1;
}

void execute() 
{
    do 
    {
        sleep(2);

        if (array_ptr[3] != 1) 
        {
            printf("From process (%d)\n", getpid());
        }
        else
        {
            // Logic for alternating between processes based on array_ptr[4]

            if (array_ptr[4] !=0 && array_ptr[4]!=2 && array_ptr[3]==1)
            {
                printf("\nThis is from C2 process (%d)\n", getpid());
                array_ptr[4] = 2;
                kill(array_ptr[2], SIGCONT);
                kill(array_ptr[1], SIGSTOP);
            }
            else if (array_ptr[4] !=0 && array_ptr[4]!=1 && array_ptr[3]==1) 
            {
                printf("This is from GC process (%d)\n", getpid());
                array_ptr[4] = 0;
                kill(array_ptr[0], SIGCONT);
                kill(array_ptr[2], SIGSTOP);
            }
                        if (array_ptr[4] !=1 && array_ptr[4]!=2 && array_ptr[3]==1) 
            {
                printf("This is from C1 (%d)\n", getpid());
                array_ptr[4] = 1;
                kill(array_ptr[1], SIGCONT);
                kill(array_ptr[0], SIGSTOP);
            }
        }
    } while (1);
}

void cleanup(int signo)
{
    // Detach shared memory
    if (shmdt(array_ptr) == -1) 
    {
        perror("shmdt");
    }

    // Deallocate the shared memory
    shmctl(shmget(IPC_PRIVATE, 5 * sizeof(int), IPC_CREAT | 0666), IPC_RMID, NULL);

    // Terminate all processes in the process group
    killpg(getpgid(getpid()), SIGTERM);

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) 
{
    int shmid;

    // Create shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, 5 * sizeof(int), IPC_CREAT | 0666)) < 0)
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

    // Initialize shared memory values
    array_ptr[3] = 0;
    array_ptr[4] = 0;
    printf("\nProcess ID of Main: %d\n", getpid());
    array_ptr[0] = getpid(); // Store the PID of the main process

    int pid1 = fork();
    int pid2 = fork();

    if (pid1 == 0 && pid2 == 0)
    {
        // Child processes
    }
    else
    {
        // Parent process
        setpgid(0, getpid());

        if (pid1 > 0 && pid2 > 0)
        {
            array_ptr[1] = pid1; // Store PID of first child process
            array_ptr[2] = pid2; // Store PID of second child process
        }

        // Set signal handlers for SIGINT and SIGTERM
        signal(SIGINT, handler);
        signal(SIGTERM, cleanup);
    }

    execute(); // Execute the main logic of the program

    return 0;
}
