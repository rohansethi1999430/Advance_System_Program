#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// Decraling the global variables

//variable to count the number of times the ctrl+c has beel pressed
int SIGNAL_TO_STOP = 0; 

//Variable to keep the track of the last time ctrc+c was called  
time_t last_sigint_time = 0; 

// handeling SIGALARM
void myAlarmHandler(int signo)
{
    
    //will reset the timmer and also change the value of global varible to keep the count of ctrl+c 
    SIGNAL_TO_STOP = 0; 
}

// Signal handler for SIGINT (Ctrl+C)
void stopper(int signo)
{
    time_t current_time;
    time(&current_time);
//treshhold for the condition is ctrl+c is pressed more than 1 time already 
//and time is less than 6 second i.e checking of ctrl +c is pressed within 6 seconds
    if (SIGNAL_TO_STOP >= 1 && ((current_time-last_sigint_time) <=6)) 
    {
        printf("\nTerminating the program as two CTRL-C were pressed within 6 seconds.\n");
        exit(0); // Exit the program
    }
    
    //incrementing the value of global value when the ctrl+c is pressed once
    SIGNAL_TO_STOP ++; 
    last_sigint_time = current_time; // Update the time of the last SIGINT
    alarm(6);

    printf("\nCTRL-C pressed. Press again within 6 seconds to terminate.\n");
}

int main(int argc, char *argv[])
{
    
    signal(SIGINT, stopper); // Register stopper() as the handler for SIGINT
    signal(SIGALRM, myAlarmHandler); // Register myAlarmHandler() as the handler for SIGALRM
//infinite loop for 
   for(;;)
    {
        printf("First CTR-C does not work- Enter another CTR-C within 6 seconds\n");
        sleep(1); // Sleep for 1 second
    }

    return 0;
}
