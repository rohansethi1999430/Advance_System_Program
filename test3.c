#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <glob.h>

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

char *trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char) *str)) str++;

    if (*str == 0) { // All spaces?
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

void split_command(char *command, char **args, int *argc) {
    char *token;
    *argc = 0;

    token = strtok(command, " ");
    while (token != NULL && *argc < MAX_TOKENS - 1) {
        args[(*argc)++] = token;
        token = strtok(NULL, " ");
    }
    args[*argc] = NULL;
}

void handle_concatenate_command(char *command) {
    char *token;
    char *files[MAX_FILES];
    int file_count = 0;

    // Tokenize the command based on the '~' character
    token = strtok(command, "~");
    while (token != NULL && file_count < MAX_FILES) {
        files[file_count++] = trim_whitespace(token);
        token = strtok(NULL, "~");
    }

    if (file_count > 1) {
        concatenate_files(files, file_count);
    } else {
        fprintf(stderr, "Error: Too few files to concatenate.\n");
    }
}

void handle_pipe_command(char *command) {
    char *commands[MAX_TOKENS];
    int command_count = 0;

    // Tokenize the command based on the '|' character
    char *token = strtok(command, "|");
    while (token != NULL && command_count < MAX_TOKENS) {
        commands[command_count++] = trim_whitespace(token);
        token = strtok(NULL, "|");
    }

    int pipefds[2 * (command_count - 1)];
    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < command_count; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Set up input from previous command
            if (i > 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Set up output to next command
            if (i < command_count - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe file descriptors
            for (int j = 0; j < 2 * (command_count - 1); j++) {
                close(pipefds[j]);
            }

            // Execute the command
            char *args[MAX_TOKENS];
            int argc = 0;
            char *arg_token = strtok(commands[i], " ");
            while (arg_token != NULL && argc < MAX_TOKENS) {
                args[argc++] = arg_token;
                arg_token = strtok(NULL, " ");
            }
            args[argc] = NULL;

            // Handle wildcard expansion using glob
            glob_t glob_result;
            memset(&glob_result, 0, sizeof(glob_result));

            for (int k = 0; k < argc; k++) {
                if (strchr(args[k], '*') != NULL) {
                    glob(args[k], GLOB_TILDE, NULL, &glob_result);
                    for (int l = 0; l < glob_result.gl_pathc; l++) {
                        args[k] = glob_result.gl_pathv[l];
                    }
                }
            }

            if (execvp(args[0], args) < 0) {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close all pipe file descriptors in the parent process
    for (int i = 0; i < 2 * (command_count - 1); i++) {
        close(pipefds[i]);
    }

    // Wait for all child processes to complete
    for (int i = 0; i < command_count; i++) {
        wait(NULL);
    }
}

void handle_redirection(char *command) {
    char *args[MAX_TOKENS];
    int argc = 0;
    char *file = NULL;
    int mode = 0; // 0 = no redirection, 1 = output, 2 = append, 3 = input

    // Detect and split redirection parts
    char *redir_pos;
    if ((redir_pos = strstr(command, ">>")) != NULL) {
        *redir_pos = '\0';
        file = redir_pos + 2;
        mode = 2;
    } else if ((redir_pos = strchr(command, '>')) != NULL) {
        *redir_pos = '\0';
        file = redir_pos + 1;
        mode = 1;
    } else if ((redir_pos = strchr(command, '<')) != NULL) {
        *redir_pos = '\0';
        file = redir_pos + 1;
        mode = 3;
    }

    trim_whitespace(file);
    split_command(command, args, &argc);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        int fd;
        if (mode == 1) {
            fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        } else if (mode == 2) {
            fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        } else if (mode == 3) {
            fd = open(file, O_RDONLY);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
        }

        if (execvp(args[0], args) < 0) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        wait(NULL);
    }
}

void handle_sequential_command(char *command) {
    char *commands[MAX_TOKENS];
    int command_count = 0;

    // Tokenize the command based on the ';' character
    char *token = strtok(command, ";");
    while (token != NULL && command_count < MAX_TOKENS) {
        commands[command_count++] = trim_whitespace(token);
        token = strtok(NULL, ";");
    }

    for (int i = 0; i < command_count; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Execute the command
            char *args[MAX_TOKENS];
            int argc = 0;
            char *arg_token = strtok(commands[i], " ");
            while (arg_token != NULL && argc < MAX_TOKENS) {
                args[argc++] = arg_token;
                arg_token = strtok(NULL, " ");
            }
            args[argc] = NULL;

            if (execvp(args[0], args) < 0) {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent process waits for the child to complete
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void handle_background_command(char *command) {
    char *args[MAX_TOKENS];
    int argc = 0;

    // Remove the '+' symbol from the command
    char *plus_pos = strchr(command, '+');
    if (plus_pos != NULL) {
        *plus_pos = '\0';
    }

    split_command(command, args, &argc);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) < 0) {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        if (num_bg_processes < MAX_BG_PROCESSES) {
            bg_process_pids[num_bg_processes++] = pid;
            printf("Process %d running in background\n", pid);
        } else {
            fprintf(stderr, "Max background processes reached\n");
        }
    }
}

void handle_foreground_command() {
    if (num_bg_processes > 0) {
        pid_t pid = bg_process_pids[--num_bg_processes];
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
        } else {
            printf("Process %d brought to foreground\n", pid);
        }
    } else {
        fprintf(stderr, "No background processes to bring to foreground\n");
    }
}
void execute_command(char *command, int *status) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        char *args[MAX_TOKENS];
        int argc = 0;
        char *arg_token = strtok(command, " ");
        while (arg_token != NULL && argc < MAX_TOKENS) {
            args[argc++] = arg_token;
            arg_token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        waitpid(pid, status, 0);
        if (WIFEXITED(*status)) {
            *status = WEXITSTATUS(*status);
        } else {
            *status = 1; // Treat non-normal exit as failure
        }
    }
}

void handle_and_operator(char **commands, int *index, int *status) {
    if (*status != 0) {
        (*index)++;
    }
}

void handle_or_operator(char **commands, int *index, int *status) {
    if (*status == 0) {
        (*index)++;
    }
}

void handle_conditional_command(char *command) {
    char *commands[MAX_TOKENS];
    int command_count = 0;

    // Split the command string based on "&&" and "||" while keeping the operators
    char *token = strtok(command, " ");
    while (token != NULL && command_count < MAX_TOKENS) {
        if (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0) {
            commands[command_count++] = token;
        } else {
            char *cmd = malloc(MAX_COMMAND_LENGTH);
            strcpy(cmd, token);
            while ((token = strtok(NULL, " ")) != NULL && strcmp(token, "&&") != 0 && strcmp(token, "||") != 0) {
                strcat(cmd, " ");
                strcat(cmd, token);
            }
            commands[command_count++] = cmd;
            continue;
        }
        token = strtok(NULL, " ");
    }

    int status = 0;
    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i], "&&") == 0) {
            handle_and_operator(commands, &i, &status);
        } else if (strcmp(commands[i], "||") == 0) {
            handle_or_operator(commands, &i, &status);
        } else {
            execute_command(commands[i], &status);
        }
    }

    // Free allocated memory
    for (int i = 0; i < command_count; i++) {
        if (commands[i] != NULL && strcmp(commands[i], "&&") != 0 && strcmp(commands[i], "||") != 0) {
            free(commands[i]);
        }
    }
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
        } else if (strchr(command, '~') != NULL) {
            handle_concatenate_command(command);
        } else if (strchr(command, '|') != NULL) {
            handle_pipe_command(command);
        } else if (strchr(command, '>') != NULL || strchr(command, '<') != NULL) {
            handle_redirection(command);
        } else if (strchr(command, ';') != NULL) {
            handle_sequential_command(command);
        } else if (strchr(command, '+') != NULL) {
            handle_background_command(command);
        } else if (strcmp(command, "fore") == 0) {
            handle_foreground_command();
        } else if (strstr(command, "&&") != NULL || strstr(command, "||") != NULL) {
            handle_conditional_command(command);
        } else {
            // General command execution
            pid_t pid = fork();

            if (pid == -1) {
                perror("fork failed");
                continue;
            } else if (pid == 0) {
                // Tokenize the command into arguments
                char *args[MAX_TOKENS];
                int argc = 0;
                char *arg_token = strtok(command, " ");
                while (arg_token != NULL && argc < MAX_TOKENS) {
                    args[argc++] = arg_token;
                    arg_token = strtok(NULL, " ");
                }
                args[argc] = NULL;

                if (execvp(args[0], args) == -1) {
                    perror("execvp failed");
                }
                exit(EXIT_FAILURE);
            } else {
                // Parent process waits for the child to complete
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }

    return 0;
}
