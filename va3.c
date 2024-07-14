#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 100

pid_t background_process = 0; // To store the PID of the last background process

void handle_redirection(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Failed to open file for writing");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("Failed to duplicate file descriptor for stdout");
                exit(1);
            }
            close(fd); // Close the original fd after dup2
            args[i] = NULL;
            args[i + 1] = NULL;
            break;
        } else if (strcmp(args[i], ">>") == 0) {
            int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (fd < 0) {
                perror("Failed to open file for appending");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("Failed to duplicate file descriptor for stdout");
                exit(1);
            }
            close(fd); // Close the original fd after dup2
            args[i] = NULL;
            args[i + 1] = NULL;
            break;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("Failed to open file for reading");
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("Failed to duplicate file descriptor for stdin");
                exit(1);
            }
            close(fd); // Close the original fd after dup2
            args[i] = NULL;
            args[i + 1] = NULL;
            break;
        }
        i++;
    }
}

void execute_multiple_pipes(char *cmd) {
    int num_pipes = 0;
    char *commands[MAX_ARG_COUNT];
    int pipefd[2 * MAX_ARG_COUNT];
    pid_t pid;

    char *cmd_token = strtok(cmd, "|");
    while (cmd_token != NULL && num_pipes < MAX_ARG_COUNT - 1) {
        commands[num_pipes++] = cmd_token;
        cmd_token = strtok(NULL, "|");
    }

    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipefd + i * 2) == -1) {
            perror("Pipe failed");
            exit(1);
        }
    }

    for (int i = 0; i < num_pipes; i++) {
        pid = fork();
        if (pid == 0) {
            if (i != 0) {
                dup2(pipefd[(i - 1) * 2], STDIN_FILENO);
            }
            if (i != num_pipes - 1) {
                dup2(pipefd[i * 2 + 1], STDOUT_FILENO);
            }

            for (int j = 0; j < 2 * num_pipes; j++) {
                close(pipefd[j]);
            }

            char *args[MAX_ARG_COUNT];
            char *token = strtok(commands[i], " \n");
            int argc = 0;
            while (token != NULL && argc < MAX_ARG_COUNT - 1) {
                args[argc++] = token;
                token = strtok(NULL, " \n");
            }
            args[argc] = NULL;

            if (execvp(args[0], args) == -1) {
                perror("Command execution failed");
                exit(1);
            }
        }
    }

    for (int i = 0; i < 2 * num_pipes; i++) {
        close(pipefd[i]);
    }

    for (int i = 0; i < num_pipes; i++) {
        wait(NULL);
    }
}

int execute_command(char **args, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(1); // If execvp fails
        }
    } else {
        // Parent process
        if (background) {
            background_process = pid;
            printf("Process %d running in background\n", pid);
            return 0;
        } else {
            int status;
            waitpid(pid, &status, 0);
            return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        }
    }
    return 0;
}

void handle_special_commands(char *input) {
    if (strchr(input, '|')) {
        execute_multiple_pipes(input);
    } else if (input[0] == '#') {
        char *filename = strtok(input + 1, " \n");
        if (filename) {
            char *args[] = {"wc", "-w", filename, NULL};
            execute_command(args, 0);
        } else {
            fprintf(stderr, "Invalid # command\n");
        }
    } else if (strchr(input, '~')) {
        char *token = strtok(input, "~ \n");
        char command[MAX_INPUT_SIZE] = "cat";
        while (token != NULL) {
            strcat(command, " ");
            strcat(command, token);
            token = strtok(NULL, "~ \n");
        }
        system(command);
    }
    // Implement additional special character handling as needed
}

void parse_and_execute(char *input) {
    char *args[MAX_ARG_COUNT];
    char *token = strtok(input, " \n");
    int argc = 0;
    int background = 0;

    while (token != NULL && argc < MAX_ARG_COUNT - 1) {
        if (strcmp(token, "+") == 0) {
            background = 1;
        } else {
            args[argc++] = token;
        }
        token = strtok(NULL, " \n");
    }
    args[argc] = NULL;

    if (argc == 0) return;

    if (strcmp(args[0], "dter") == 0) {
        exit(0);
    } else if (strcmp(args[0], "dtex") == 0) {
        system("killall -u $USER");
        exit(0);
    } else if (strcmp(args[0], "addmb") == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("/bin/bash", "bash", (char *)NULL);
            exit(0);
        }
    } else if (strcmp(args[0], "fore") == 0) {
        if (background_process != 0) {
            int status;
            printf("Bringing process %d to foreground\n", background_process);
            waitpid(background_process, &status, 0);
            background_process = 0;
        } else {
            fprintf(stderr, "No background process to bring to foreground\n");
        }
    } else {
        handle_redirection(args);
        execute_command(args, background);
    }
}

void parse_and_execute_multiple(char *input) {
    char *saveptr;
    char *cmd = strtok_r(input, "&&", &saveptr);
    while (cmd != NULL) {
        // Trim leading and trailing spaces
        char trimmed_cmd[MAX_INPUT_SIZE];
        sscanf(cmd, " %1023[^\t\n]", trimmed_cmd);

        if (strlen(trimmed_cmd) > 0) {
            parse_and_execute(trimmed_cmd);
        }

        // Move to the next command
        cmd = strtok_r(NULL, "&&", &saveptr);
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    int original_stdout = dup(STDOUT_FILENO); // Save the original stdout
    int original_stdin = dup(STDIN_FILENO); // Save the original stdin

    while (1) {
        // Restore original stdout and stdin
        dup2(original_stdout, STDOUT_FILENO);
        dup2(original_stdin, STDIN_FILENO);

        printf("\nminibash$ ");
        fflush(stdout); // Ensure the prompt is printed

        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Failed to read input");
            exit(1);
        }
        input[strcspn(input, "\n")] = '\0';  // Remove the newline character

        handle_special_commands(input);
        parse_and_execute_multiple(input);
    }

    close(original_stdout); // Close the duplicated file descriptor
    close(original_stdin); // Close the duplicated file descriptor
    return 0;
}
