#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 10
#define MAX_PIPES 4

void execute_command(char *command);
void handle_special_characters(char *command);
void handle_dter();
void handle_dtex();
void handle_addmb();
void handle_count_words(char *filename);
void handle_concatenate_files(char *command);
void handle_background(char **args);
void handle_pipe(char *command);
void handle_redirection(char *command);
void handle_sequential(char *command);
void handle_conditional(char *command, int mode);

int main() {
    char command[256];

    while (1) {
        printf("minibash$ ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        // Remove trailing newline character
        command[strcspn(command, "\n")] = 0;

        // Exit on 'exit' command
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Execute command
        execute_command(command);
    }

    return 0;
}

void execute_command(char *command) {
    // Check for special commands
    if (strncmp(command, "dter", 4) == 0) {
        handle_dter();
    } else if (strncmp(command, "dtex", 4) == 0) {
        handle_dtex();
    } else if (strncmp(command, "addmb", 5) == 0) {
        handle_addmb();
    } else {
        // Handle special characters
        handle_special_characters(command);
    }
}

void handle_special_characters(char *command) {
    if (strchr(command, ';') != NULL) {
        handle_sequential(command);
    } else if (strchr(command, '>') != NULL || strchr(command, '<') != NULL) {
        handle_redirection(command);
    } else {
        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(command, " ");
        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        // Handle other special characters
        if (args[0][0] == '#') {
            handle_count_words(args[1]);
        } else if (strchr(command, '~') != NULL) {
            handle_concatenate_files(command);
        } else if (strchr(command, '+') != NULL) {
            handle_background(args);
        } else if (strchr(command, '|') != NULL) {
            handle_pipe(command);
        } else if (strstr(command, "&&") != NULL) {
            handle_conditional(command, 1);
        } else if (strstr(command, "||") != NULL) {
            handle_conditional(command, 0);
        } else {
            // General command execution
            pid_t pid = fork();

            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // In child process
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            } else {
                // In parent process
                wait(NULL);
            }
        }
    }
}

void handle_sequential(char *command) {
    char *cmds[MAX_ARGS];
    int num_cmds = 0;

    char *token = strtok(command, ";");
    while (token != NULL && num_cmds < MAX_ARGS) {
        cmds[num_cmds++] = token;
        token = strtok(NULL, ";");
    }

    for (int i = 0; i < num_cmds; i++) {
        execute_command(cmds[i]);
    }
}

void handle_redirection(char *command) {
    char *args[MAX_ARGS];
    int argc = 0;
    char *token = strtok(command, " ");
    char *file = NULL;
    int mode = 0; // 0 = no redirection, 1 = output, 2 = append, 3 = input

    while (token != NULL && argc < MAX_ARGS - 1) {
        if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            file = token;
            mode = 1;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " ");
            file = token;
            mode = 2;
        } else if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            file = token;
            mode = 3;
        } else {
            args[argc++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

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

// Remaining function implementations...

void handle_dter() {
    printf("Terminating current minibash terminal...\n");
    exit(0);
}

void handle_dtex() {
    printf("Terminating all minibash terminals...\n");
    exit(0);
}

void handle_addmb() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // Child process to start a new minibash
        execlp("./minibash", "minibash", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL);
    }
}

void handle_count_words(char *filename) {
    if (filename == NULL) {
        fprintf(stderr, "Error: No filename provided for word count.\n");
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        execlp("wc", "wc", "-w", filename, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void handle_concatenate_files(char *command) {
    char *files[MAX_ARGS];
    int argc = 0;
    char *token = strtok(command, "~");

    while (token != NULL && argc < MAX_ARGS) {
        files[argc++] = token;
        token = strtok(NULL, "~");
    }
    files[argc] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        execlp("cat", "cat", files[0], files[1], files[2], files[3], NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void handle_background(char **args) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        // In child process, remove the '+' character
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "+") == 0) {
                args[i] = NULL;
                break;
            }
        }
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // In parent process, do not wait for child process
        printf("Running %s in the background...\n", args[0]);
    }
}

void handle_pipe(char *command) {
    int pipefds[2 * MAX_PIPES];
    char *cmds[MAX_PIPES + 1];
    int num_cmds = 0;

    char *token = strtok(command, "|");
    while (token != NULL && num_cmds < MAX_PIPES + 1) {
        cmds[num_cmds++] = token;
        token = strtok(NULL, "|");
    }

    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (i > 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            if (i < num_cmds - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipefds[j]);
            }

            char *cmd_args[MAX_ARGS];
            int argc = 0;
            char *arg_token = strtok(cmds[i], " ");
            while (arg_token != NULL && argc < MAX_ARGS - 1) {
                cmd_args[argc++] = arg_token;
                arg_token = strtok(NULL, " ");
            }
            cmd_args[argc] = NULL;

            if (execvp(cmd_args[0], cmd_args) < 0) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipefds[i]);
    }

    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

void handle_conditional(char *command, int mode) {
    char *cmds[2];
    cmds[0] = strtok(command, mode == 1 ? "&&" : "||");
    cmds[1] = strtok(NULL, mode == 1 ? "&&" : "||");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        if (execlp(cmds[0], cmds[0], NULL) < 0) {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    } else {
        int status;
        wait(&status);
        if ((mode == 1 && WIFEXITED(status) && WEXITSTATUS(status) == 0) ||
            (mode == 0 && WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
            execute_command(cmds[1]);
        }
    }
}
