#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void action() {
    printf("Switching\n");
}

int main(int argc, char *argv[]) {
    pid_t pid;
    if ((pid = fork()) > 0) { // Parent
        signal(SIGINT, action);
        while (1) {
            printf("Parent is running\n");
            sleep(2); // Sleep for 2 seconds
            kill(pid, SIGINT); // Sends SIGINT to child process
            pause(); // Pauses until a signal is received
        }
    } else { // Child
        signal(SIGINT, action);
        while (1) {
            pause(); // Pauses until a signal is received
            printf("Child is running\n");
            sleep(2); // Sleep for 2 seconds
            kill(getppid(), SIGINT); // Sends SIGINT to parent process
        }
    }
    return 0;
}