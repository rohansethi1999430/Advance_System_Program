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
#define MAX_FILES 4
#define PID_FILE "/tmp/minibash_pids.txt"

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

void concatenate_files(char *files[], int file_count) {
    for (int i = 0; i < file_count; i++) {
        FILE *file = fopen(files[i], "r");
        if (file == NULL) {
            perror("fopen failed");
            continue;
        }
        char buffer[BUFFER_SIZE];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, n, stdout);
        }
        fclose(file);
    }
}



void add_pid_to_file(pid_t pid) {
    FILE *file = fopen(PID_FILE, "a");
    if (file == NULL) {
        perror("fopen failed");
        return;
    }
    fprintf(file, "%d\n", pid);
    fclose(file);
}

void remove_pid_from_file(pid_t pid) {
    FILE *file = fopen(PID_FILE, "r");
    if (file == NULL) {
        perror("fopen failed");
        return;
    }

    FILE *temp_file = fopen("/tmp/temp_pids.txt", "w");
    if (temp_file == NULL) {
        perror("fopen failed");
        fclose(file);
        return;
    }

    int current_pid;
    while (fscanf(file, "%d", &current_pid) != EOF) {
        if (current_pid != pid) {
            fprintf(temp_file, "%d\n", current_pid);
        }
    }

    fclose(file);
    fclose(temp_file);

    remove(PID_FILE);
    rename("/tmp/temp_pids.txt", PID_FILE);
}

void kill_all_minibash() {
    FILE *file = fopen(PID_FILE, "r");
    if (file == NULL) {
        perror("fopen failed");
        return;
    }

    int pid;
    while (fscanf(file, "%d", &pid) != EOF) {
        kill(pid, SIGKILL);
    }

    fclose(file);
    remove(PID_FILE);
}

int main() {
    char command[MAX_COMMAND_LENGTH]; // Buffer for user input
    pid_t my_pid = getpid();

    // Register signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, handler);

    // Add the current PID to the PID file
    add_pid_to_file(my_pid);

    char *bg_processes[10];

    for (;;) {
        printf("minibash$ ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        command[strcspn(command, "\n")] = 0;  // Remove the newline character

        if (strcmp(command, "dter") == 0) {
            // Remove the current PID from the PID file
            remove_pid_from_file(my_pid);
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "dtex") == 0) {
            kill_all_minibash();
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "addmb") == 0) {
            // Create a new minibash process
            pid_t pid = fork();

            if (pid == -1) {
                // Fork failed
                perror("fork failed");
                continue;
            } else if (pid == 0) {
                // Child process
                execlp("./ra3", "ra3", NULL); // Assuming minibash executable is in the current directory
                // If execlp fails
                perror("execlp failed");
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                // Wait for the child process to complete
                int status;
                waitpid(pid, &status, 0);
            }
        } else if (command[0] == '#') {
            // Extract the filename
            char *filename = trim_whitespace(command + 1);  // Skip the "#"
            printf("\nFile name: '%s'\n", filename);

            // Create a child process
            pid_t pid = fork();

            if (pid == -1) {
                // Fork failed
                perror("fork failed");
                continue;
            } else if (pid == 0) {
                // Child process
                // Execute the wc -w command
                execlp("wc", "wc", "-w", filename, NULL);
                // If execlp fails
                perror("execlp failed");
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                // Wait for the child process to complete
                int status;
                waitpid(pid, &status, 0);
            }
        } else {
            printf("Unknown command: %s\n", command);
        }
    }

    return 0;
}
