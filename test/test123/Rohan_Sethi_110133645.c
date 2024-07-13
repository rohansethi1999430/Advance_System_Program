#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


//function to create pipes
void create_pipe(int pipefd[2]) {
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }
}

//function to execute the cat command
void execute_cat(int pipe1[2]) {
    if (fork() != 0) {
        // Parent process
        wait(NULL);
    } else {
        // Child process
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe1[1]);
        execlp("cat", "cat", "sample.txt", NULL);
        perror("execlp cat failed");
        exit(1);
    }
}

void execute_grep(int pipe1[2], int pipe2[2]) {
    if (fork() != 0) {
        // Parent process
        close(pipe1[0]);
        close(pipe1[1]);
    } else {
        // Child process
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        execlp("grep", "grep", "to", NULL);
        perror("execlp grep failed");
        exit(1);
    }
}
//function to execute the wc command
void execute_wc(int pipe2[2]) {
    if (fork() != 0) {
        // Parent process
        close(pipe2[0]);
        close(pipe2[1]);
    } else {
        // Child process
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        close(pipe2[1]);
        execlp("wc", "wc", "-w", NULL);
        perror("execlp wc failed");
        exit(1);
    }
}

void wait_for_children() {
int i = 0;

//do-while loop to make the parent wait for all 3 childern
do {
    wait(NULL);
    i++;
} while (i < 3);

}

int main() {
    //declaring two pipes
    int pipe1[2], pipe2[2];

    // Create pipe 1
    create_pipe(pipe1);
    // Execute 'cat sample.txt'
    execute_cat(pipe1);

    // Create pipe 2
    create_pipe(pipe2);
    // Execute 'grep to'
    execute_grep(pipe1, pipe2);

    // Execute 'wc -w'
    execute_wc(pipe2);

    // Wait for all child processes to finish
    wait_for_children();

    return 0;
}
