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
    if (num_bg_processes == 0)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        kill(bg_process_pids[num_bg_processes - 1], SIGKILL);
        num_bg_processes--;
    }
}

void concatenate_files(char *files[], int file_count) {
    int i = 0;
    if (file_count > 0) {
        do {
            FILE *file = fopen(files[i], "r");
            if (file == NULL) {
                perror("fopen failed");
            } else {
                char buffer[BUFFER_SIZE];
                size_t n;
                while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    fwrite(buffer, 1, n, stdout);
                }
                fclose(file);
            }
            i++;
        } while (i < file_count);
    }
}

void add_pid_to_file(pid_t pid) {
    FILE *file = fopen(PID_FILE, "a");
    if (file != NULL) {
        fprintf(file, "%d\n", pid);
        fclose(file);
    } else {
        perror("fopen failed");
    }
}

void remove_pid_from_file(pid_t pid) {
    FILE *file = fopen(PID_FILE, "r");
    FILE *temp_file = fopen("/tmp/temp_pids.txt", "w");

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

    int pid;
    while (fscanf(file, "%d", &pid) != EOF) {
        kill(pid, SIGKILL);
    }

    fclose(file);
    remove(PID_FILE);
}

char *trim_whitespace(char *str) {
    char *end;

    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

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
            if (i > 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            if (i < command_count - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < 2 * (command_count - 1); j++) {
                close(pipefds[j]);
            }

            char *args[MAX_TOKENS];
            int argc = 0;
            char *arg_token = strtok(commands[i], " ");
            while (arg_token != NULL && argc < MAX_TOKENS) {
                args[argc++] = arg_token;
                arg_token = strtok(NULL, " ");
            }
            args[argc] = NULL;

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

    for (int i = 0; i < 2 * (command_count - 1); i++) {
        close(pipefds[i]);
    }

    for (int i = 0; i < command_count; i++) {
        wait(NULL);
    }
}

void handle_redirection(char *command) {
    char *args[MAX_TOKENS];
    int argc = 0;
    char *file = NULL;
    int mode = 0;

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
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void handle_background_command(char *command) {
    char *args[MAX_TOKENS];
    int argc = 0;

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
        if (execvp(args[0], args) < 0) {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    } else {
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
        waitpid(pid, status, 0);
        if (WIFEXITED(*status)) {
            *status = WEXITSTATUS(*status);
        } else {
            *status = 1;
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

    for (int i = 0; i < command_count; i++) {
        if (commands[i] != NULL && strcmp(commands[i], "&&") != 0 && strcmp(commands[i], "||") != 0) {
            free(commands[i]);
        }
    }
}

void handle_input(char *command) {
    char temp_command[MAX_COMMAND_LENGTH] = "";
    char *ptr = command;

    while (*ptr != '\0') {
        if (*ptr == '>' && *(ptr + 1) == '>') {
            strcat(temp_command, " >> ");
            ptr += 2;
        } else if (*ptr == '>' || *ptr == '<' || *ptr == '|' || *ptr == ';' || *ptr == '+') {
            strncat(temp_command, " ", 1);
            strncat(temp_command, ptr, 1);
            strncat(temp_command, " ", 1);
            ptr++;
        } else if (*ptr == '&' && *(ptr + 1) == '&') {
            strcat(temp_command, " && ");
            ptr += 2;
        } else if (*ptr == '|' && *(ptr + 1) == '|') {
            strcat(temp_command, " || ");
            ptr += 2;
        } else {
            strncat(temp_command, ptr, 1);
            ptr++;
        }
    }
    strcpy(command, temp_command);
}

int main() {
    char command[MAX_COMMAND_LENGTH];
    pid_t my_pid = getpid();

    signal(SIGINT, handler);

    add_pid_to_file(my_pid);

    for (;;) {
        printf("minibash$ ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        command[strcspn(command, "\n")] = 0;

        handle_input(command);

        int cmd_type = -1;
        if (strcmp(command, "dter") == 0) cmd_type = 0;
        else if (strcmp(command, "dtex") == 0) cmd_type = 1;
        else if (command[0] == '#') cmd_type = 2;
        else if (strchr(command, '~') != NULL) cmd_type = 3;
        else if (strstr(command, "&&") != NULL || strstr(command, "||") != NULL) cmd_type = 4;
        else if (strchr(command, '|') != NULL) cmd_type = 5;
        else if (strchr(command, '>') != NULL || strchr(command, '<') != NULL) cmd_type = 6;
        else if (strchr(command, ';') != NULL) cmd_type = 7;
        else if (strchr(command, '+') != NULL) cmd_type = 8;
        else if (strcmp(command, "fore") == 0) cmd_type = 9;
        else cmd_type = 10;

        switch (cmd_type) {
            case 0:
                remove_pid_from_file(my_pid);
                exit(EXIT_SUCCESS);
            case 1:
                kill_all_minibash();
                exit(EXIT_SUCCESS);
            case 2: {
                char *filename = trim_whitespace(command + 1);
                printf("\nFile name: '%s'\n", filename);

                pid_t pid = fork();

                if (pid == -1) {
                    perror("fork failed");
                    continue;
                } else if (pid == 0) {
                    execlp("wc", "wc", "-w", filename, NULL);
                    perror("execlp failed");
                    exit(EXIT_FAILURE);
                } else {
                    int status;
                    waitpid(pid, &status, 0);
                }
                break;
            }
            case 3:
                handle_concatenate_command(command);
                break;
            case 4:
                handle_conditional_command(command);
                break;
            case 5:
                handle_pipe_command(command);
                break;
            case 6:
                handle_redirection(command);
                break;
            case 7:
                handle_sequential_command(command);
                break;
            case 8:
                handle_background_command(command);
                break;
            case 9:
                handle_foreground_command();
                break;
            case 10: {
                pid_t pid = fork();

                if (pid == -1) {
                    perror("fork failed");
                    continue;
                } else if (pid == 0) {
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
                    int status;
                    waitpid(pid, &status, 0);
                }
                break;
            }
        }
    }

    return 0;
}
