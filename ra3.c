#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h> 


#define MAX_COMMAND_LENGTH 1000
#define BUFFER_SIZE 1024
#define MAX_TOKENS 10
#define MAX_BG_PROCESSES 10

int num_bg_processes = 0;
pid_t bg_process_pids[MAX_BG_PROCESSES];


void handler(int signo) 
{
    // Check if there are any background processes running
    if (num_bg_processes > 0)
    {
        // Kill the last background process and decrement the counter
        kill(bg_process_pids[num_bg_processes - 1], SIGKILL);
        num_bg_processes--;
    }
    else
    {
        // If no background processes are running, exit the program
        exit(EXIT_SUCCESS);
    }
}

int main(){

    char command[MAX_COMMAND_LENGTH]; // Buffer for user input

    // Register signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, handler);

    char *bg_processes[10]; 
}