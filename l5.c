#include <stdio.h>
#include <stdlib.h> 
#include <sys/signal.h> 

int SIGNAL_TO_STOP = 0; // Global variable to control program termination

// Signal handler for SIGALRM
void myAlarmHandler(int signo)
{
    SIGNAL_TO_STOP = 2; // Set SIGNAL_TO_STOP to 2 indicating the program should stop
}

// Signal handler for SIGINT (Ctrl+C)
void stopper(int signo)
{
    if(SIGNAL_TO_STOP == 1) // If SIGNAL_TO_STOP is 1, indicating second Ctrl+C press
    {
        exit(0); // Exit the program
    }
    if(SIGNAL_TO_STOP == 2) // If SIGNAL_TO_STOP is 2, indicating SIGALRM was received
    {
        SIGNAL_TO_STOP = 0; // Reset SIGNAL_TO_STOP

        kill(getpid(), SIGSTOP); // Send SIGSTOP to the current process
    }

    SIGNAL_TO_STOP = 1; // Set SIGNAL_TO_STOP to 1 indicating first Ctrl+C press

    alarm(6); // Schedule a SIGALRM to be sent after 6 seconds
}

int main(int argc, char *argv[])
{
    signal(SIGINT, stopper); // Register stopper() as the handler for SIGINT

    signal(SIGALRM, myAlarmHandler); // Register myAlarmHandler() as the handler for SIGALRM

   for(;;)
    {
        printf("\nOnly one CTR-C does not work on this program\n");

        sleep(1); // Sleep for 1 second
    }

    return 0;
}
