#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipe1[2], pipe2[2];

    // Create pipe 1
    if (pipe(pipe1) == -1) {
        perror("pipe1 failed");
        exit(1);
    }

    // Createchild 1 
if (fork() != 0) {
    //parent process will wait for the child 
    wait(NULL);
} else {
    // Child process logic
    // Redirect stdout to pipe1 write end
    dup2(pipe1[1], STDOUT_FILENO);
    // Close unused pipe ends
    close(pipe1[0]);
    close(pipe1[1]);

    // 'cat sample.txt'
    execlp("cat", "cat", "sample.txt", NULL);
    perror("execlp cat failed");
    exit(1);
}

    // Create second pipe
    if (pipe(pipe2) == -1) {
        perror("pipe2 failed");
        exit(1);
    }

    // Create second child process
if (fork() != 0) {
    // Parent process logic
    // Parent can perform other tasks or wait for the child process
} else {
    
    // Redirec stdin to pipe1 read
    dup2(pipe1[0], STDIN_FILENO);
    // Redire stdout to pipe2 write
    dup2(pipe2[1], STDOUT_FILENO);
    // Clossing pipes
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    // 'grep to' command calling
    execlp("grep", "grep", "to", NULL);
    perror("execlp grep failed");
    exit(1);
}
    // Closeing pipes
    close(pipe1[0]);
    close(pipe1[1]);

    // Create child 3
if (fork() != 0) {
    // Parent process logic
    // Parent can perform other tasks or wait for the child process
} else {
    // Redirect stdin to pipe2 read calling dup2 call
    dup2(pipe2[0], STDIN_FILENO);
    // Closeing pipes
    close(pipe2[0]);
    close(pipe2[1]);

    // calling 'wc -w'
    execlp("wc", "wc", "-w", NULL);
    perror("execlp wc failed");
    exit(1);
}


    // Close unused pipe ends in the parent process
    close(pipe2[0]);
    close(pipe2[1]);

    // Wait for all child processes to finish
int i = 0;
do {
    wait(NULL);
    i++;
} while (i < 3);

    return 0;
}
