#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <ctype.h>




#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 100
#define MAX_BG_PROCESSES 100
#define MAX_COMMAND_LENGTH 1024
#define MAX_TOKENS 100

int isprogramRunning = 22;



// Function prototypes
void handler(int signum);
void add_pid_to_file(pid_t pid);
void remove_pid_from_file(pid_t pid);
void kill_all_minibash();
char *trim_whitespace(char *str);
void handle_conditional_command(char *command);
void handle_pipe_command(char *command);
void handle_sequential_command(char *command);
void handle_foreground_command();
void handle_concatenate_command(char *command);
void handle_redirection(char **args, int *saved_stdin, int *saved_stdout);
void restore_redirection(int saved_stdin, int saved_stdout);
void expand_wildcards(char **args, char **expanded_args, int *expanded_argc);
void handle_background_command(char *command);
int execute_command(char **args, int background);
int execute_pipeline(char *commands[][MAX_ARG_COUNT], int command_count);
void parse_and_execute_multiple(char *input);
int parse_and_execute(char *input);
void handle_special_commands(char *input);
void split_command(char *command, char **args, int *argc);



pid_t bg_process_pids[MAX_BG_PROCESSES];
int num_bg_processes = 0;
volatile sig_atomic_t stop = 0;

void handler(int signum) {
 exit(0);
}

void add_pid_to_file(pid_t pid) {
    // Implementation for adding PID to file
}

void remove_pid_from_file(pid_t pid) {
    // Implementation for removing PID from file
}

void kill_all_minibash() {
    // Implementation for killing all minibash processes
}

char *trim_whitespace(char *str) {
    char *end;

    // Replace the first while loop with a for loop
    for (; isspace((unsigned char) *str); str++);

    if (*str == 0) return str;

    end = str + strlen(str) - 1;

    // Replace the second while loop with a for loop
    for (; end > str && isspace((unsigned char) *end); end--);

    end[1] = '\0';
    return str;
}



void handle_conditional_command(char *command) {
    parse_and_execute_multiple(command);
}

void handle_pipe_command(char *command) {
    char *cmd_pipe[MAX_ARG_COUNT][MAX_ARG_COUNT] = {NULL};
    char *saveptr_pipe;
    char *cmd = strtok_r(command, "|", &saveptr_pipe);
    int cmd_count = 0;

    while (cmd != NULL && cmd_count < MAX_ARG_COUNT - 1) {
        char *saveptr_cmd;
        char *arg = strtok_r(cmd, " \n", &saveptr_cmd);
        int arg_count = 0;

        while (arg != NULL && arg_count < MAX_ARG_COUNT - 1) {
            cmd_pipe[cmd_count][arg_count++] = arg;
            arg = strtok_r(NULL, " \n", &saveptr_cmd);
        }
        cmd_pipe[cmd_count++][arg_count] = NULL;
        cmd = strtok_r(NULL, "|", &saveptr_pipe);
    }

    execute_pipeline(cmd_pipe, cmd_count);
}




void handle_sequential_command(char *command) {
    char *saveptr;
    char *cmd = strtok_r(command, ";", &saveptr);

    while (cmd != NULL) {
        parse_and_execute(cmd);
        cmd = strtok_r(NULL, ";", &saveptr);
    }
}




void handle_foreground_command() {
    if (num_bg_processes > 0) {
        pid_t pid = bg_process_pids[--num_bg_processes];
        int status;
        printf("Bringing process %d to foreground\n", pid);
        // Bring the background process to the foreground
        tcsetpgrp(STDIN_FILENO, pid);
        kill(pid, SIGCONT);
        waitpid(pid, &status, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
    } else {
        fprintf(stderr, "No background process to bring to foreground\n");
    }
}




void handle_concatenate_command(char *command) {
    char *saveptr;
    char *token = strtok_r(command, "~ \n", &saveptr);
    char *args[MAX_ARG_COUNT];
    int argc = 0;

    while (token != NULL && argc < MAX_ARG_COUNT - 1) {
        args[argc++] = token;
        token = strtok_r(NULL, "~ \n", &saveptr);
    }
    args[argc] = NULL;

    if (argc > 1) {
        char *cmd_args[MAX_ARG_COUNT];
        cmd_args[0] = "cat";
        for (int i = 0; i < argc; i++) {
            cmd_args[i + 1] = args[i];
        }
        cmd_args[argc + 1] = NULL;
        execute_command(cmd_args, 0);
    } else {
        fprintf(stderr, "Invalid ~ command\n");
    }
}




void handle_redirection(char **args, int *saved_stdin, int *saved_stdout) {
    int i = 0;
    *saved_stdin = dup(STDIN_FILENO);
    *saved_stdout = dup(STDOUT_FILENO);
    int redirect_in = -1, redirect_out = -1, redirect_append = -1;

for (; args[i] != NULL; i++) {
    if (strcmp(args[i], ">") == 0) {
        redirect_out = i;
        continue;
    } 
    if (strcmp(args[i], ">>") == 0) {
        redirect_append = i;
        continue;
    } 
    if (strcmp(args[i], "<") == 0) {
        redirect_in = i;
    }
}

    if (redirect_out != -1) {
        int fd = open(args[redirect_out + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Failed to open file for writing");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Failed to duplicate file descriptor for stdout");
            exit(1);
        }
        close(fd);
        args[redirect_out] = NULL;
    } else if (redirect_append != -1) {
        int fd = open(args[redirect_append + 1], O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd < 0) {
            perror("Failed to open file for appending");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("Failed to duplicate file descriptor for stdout");
            exit(1);
        }
        close(fd);
        args[redirect_append] = NULL;
    } else if (redirect_in != -1) {
        int fd = open(args[redirect_in + 1], O_RDONLY);
        if (fd < 0) {
            perror("Failed to open file for reading");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("Failed to duplicate file descriptor for stdin");
            exit(1);
        }
        close(fd);
        args[redirect_in] = NULL;
    }
}




void restore_redirection(int saved_stdin, int saved_stdout) {
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdin);
    close(saved_stdout);
}




void expand_wildcards(char **args, char **expanded_args, int *expanded_argc) {
    glob_t globbuf;
    int i = 0;
    *expanded_argc = 0;

    while (args[i] != NULL) {
        if (strchr(args[i], '*') != NULL || strchr(args[i], '?') != NULL) {
            // Handle wildcard expansion
            if (glob(args[i], 0, NULL, &globbuf) == 0) {
                for (size_t j = 0; j < globbuf.gl_pathc; j++) {
                    expanded_args[(*expanded_argc)++] = strdup(globbuf.gl_pathv[j]);
                }
                globfree(&globbuf);
            } else {
                expanded_args[(*expanded_argc)++] = args[i];
            }
        } else {
            expanded_args[(*expanded_argc)++] = args[i];
        }
        i++;
    }
    expanded_args[*expanded_argc] = NULL;
}




void split_command(char *command, char **args, int *argc) {
    *argc = 0;
    char *token = strtok(command, " \n");
    while (token != NULL) {
        args[(*argc)++] = token;
        token = strtok(NULL, " \n");
    }
    args[*argc] = NULL;
}




void handle_background_command(char *command) {
        char *args[MAX_ARG_COUNT];
        int argc = 0;

        // Remove the '+' symbol from the command
        char *plus_pos = strchr(command, '+');
        if (plus_pos != NULL && isprogramRunning == 22) {
            *plus_pos = '\0';
        }

split_command(command, args, &argc);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

        if (pid == 0 && isprogramRunning == 22) {
            // Child process
            int saved_stdin, saved_stdout;
            handle_redirection(args, &saved_stdin, &saved_stdout); // Ensure redirection is handled in the child process
            if (execvp(args[0], args) < 0) {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
            restore_redirection(saved_stdin, saved_stdout);
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




int execute_command(char **args, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        int saved_stdin, saved_stdout;
        handle_redirection(args, &saved_stdin, &saved_stdout); // Ensure redirection is handled in the child process

        char *expanded_args[MAX_ARG_COUNT];
        int expanded_argc;
        expand_wildcards(args, expanded_args, &expanded_argc);

        if (background) {
            // Detach from parent to run in the background
            if (setpgid(0, 0) < 0) {
                perror("setpgid failed");
                exit(1);
            }
        }

        if (execvp(expanded_args[0], expanded_args) == -1) {
            perror("execvp failed");
            exit(1); // If execvp fails
        }
        restore_redirection(saved_stdin, saved_stdout);
    } else {
        // Parent process
        if (background) {
            bg_process_pids[num_bg_processes++] = pid;
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





int execute_pipeline(char *commands[][MAX_ARG_COUNT], int command_count) {
    int i;
    int pipefds[2 * (command_count - 1)];
    
    for (i = 0; i < (command_count - 1); i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            exit(1);
        }
    }
    
    int j = 0;
    for (i = 0; i < command_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            if (i < (command_count - 1)) {
                if (dup2(pipefds[j + 1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }
            
            if (i > 0) {
                if (dup2(pipefds[j - 2], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }
            
            for (int k = 0; k < 2 * (command_count - 1); k++) {
                close(pipefds[k]);
            }

            char *expanded_args[MAX_ARG_COUNT];
            int expanded_argc;
            expand_wildcards(commands[i], expanded_args, &expanded_argc);

            if (execvp(expanded_args[0], expanded_args) < 0) {
                perror("execvp");
                exit(1);
            }
        }
        j += 2;
    }
    
    for (i = 0; i < 2 * (command_count - 1); i++) {
        close(pipefds[i]);
    }
    
    for (i = 0; i < command_count; i++) {
        wait(NULL);
    }
    
    return 0;
}




void parse_and_execute_multiple(char *input) {
    char *saveptr_and;
    char *cmd_and = strtok_r(input, "&&", &saveptr_and);
    int overall_status = 0;

for (; cmd_and != NULL; cmd_and = strtok_r(NULL, "&&", &saveptr_and)) {
    char *saveptr_or;
    char *cmd_or = strtok_r(cmd_and, "||", &saveptr_or);
    int cmd_status = parse_and_execute(cmd_or);

    // Handle the rest of the || commands if the first one fails
    for (; cmd_status != 0 && (cmd_or = strtok_r(NULL, "||", &saveptr_or)); ) {
        cmd_status = parse_and_execute(cmd_or);
    }

    overall_status = cmd_status;

    // If the current && command failed, break the loop
    if (cmd_status != 0) {
        break;
    }
}

}


int parse_and_execute(char *input) {
    char *args[MAX_ARG_COUNT];
    int argc = 0;
    int background = 0;

    // Tokenize the input command
    for (char *token = strtok(input, " \n"); token != NULL; token = strtok(NULL, " \n")) {
        if (strcmp(token, "+") == 0) {
            background = 1;
        } else {
            args[argc++] = token;
        }
    }
    args[argc] = NULL;

    if (argc == 0) {
        return 0; // Return success if no command is given
    }

    if (strcmp(args[0], "dter") == 0) {
        exit(0);
    }

    if (strcmp(args[0], "fore") == 0) {
        if (num_bg_processes > 0) {
            pid_t pid = bg_process_pids[--num_bg_processes];
            int status;
            printf("Bringing process %d to foreground\n", pid);
            // Bring the background process to the foreground
            tcsetpgrp(STDIN_FILENO, pid);
            kill(pid, SIGCONT);
            waitpid(pid, &status, WUNTRACED);
            tcsetpgrp(STDIN_FILENO, getpid());
        } else {
            fprintf(stderr, "No background process to bring to foreground\n");
        }
        return 0; // Default return success after handling "fore" command
    }

    // Handle other commands
    int saved_stdin, saved_stdout;
    handle_redirection(args, &saved_stdin, &saved_stdout);
    int status = execute_command(args, background);
    restore_redirection(saved_stdin, saved_stdout);
    return status; // Return the status of the command execution
}



void handle_special_commands(char *input) {
    if (input[0] == '#') {
        char *filename = strtok(input + 1, " \n");
        if (filename) {
            char *args[] = {"wc", "-w", filename, NULL};
            execute_command(args, 0);
        } else {
            fprintf(stderr, "Invalid # command\n");
        }
    } else if (strchr(input, '~')) {
        char *saveptr;
        char *token = strtok_r(input, "~ \n", &saveptr);
        char *args[MAX_ARG_COUNT];
        int argc = 0;

        while (token != NULL && argc < MAX_ARG_COUNT - 1) {
            args[argc++] = token;
            token = strtok_r(NULL, "~ \n", &saveptr);
        }
        args[argc] = NULL;

        if (argc > 1) {
            char *cmd_args[MAX_ARG_COUNT];
            cmd_args[0] = "cat";
            for (int i = 0; i < argc; i++) {
                cmd_args[i + 1] = args[i];
            }
            cmd_args[argc + 1] = NULL;
            execute_command(cmd_args, 0);
        } else {
            fprintf(stderr, "Invalid ~ command\n");
        }
    }
    // Add more special command handling as needed
}





int main() {
    char command[MAX_COMMAND_LENGTH]; // Buffer for user input
    pid_t my_pid = getpid();

    // Register signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, handler);

    // Add the current PID to the PID file
    add_pid_to_file(my_pid);

    for (;;) {
        if (stop) {
            printf("\nStopping minibash.\n");
            break;
        }

        printf("\nminibash$ ");
        fflush(stdout);
        
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            if (feof(stdin)) {
                clearerr(stdin); // Clear EOF flag to continue reading
                printf("\n");
                continue;
            } else {
                perror("fgets failed");
                continue;
            }
        }

        command[strcspn(command, "\n")] = 0;  // Remove the newline character

        if (strcmp(command, "dter") == 0) {
            // Remove the current PID from the PID file
            remove_pid_from_file(my_pid);
            exit(EXIT_SUCCESS);
        } else if (command[0] == '#') {
            // Extract the filename
            char *filename = trim_whitespace(command + 1);  // Skip the "#"
            printf("\nFile name: '%s'\n", filename);

            // Create a child process
            pid_t pid = fork();

            if (pid == -1 && isprogramRunning == 22) {
                // Fork failed
                perror("fork failed");
                continue;
            } else if (pid == 0 && isprogramRunning == 22) {
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
        } else if (strstr(command, "&&") != NULL || strstr(command, "||") != NULL) {
            handle_conditional_command(command);
        } else if (strchr(command, '|') != NULL) {
            handle_pipe_command(command);
        } else if (strchr(command, '>') != NULL || strchr(command, '<') != NULL) {
            char *args[MAX_ARG_COUNT];
            int argc;
            split_command(command, args, &argc);
            int saved_stdin, saved_stdout;
            handle_redirection(args, &saved_stdin, &saved_stdout);
            execute_command(args, 0);
            restore_redirection(saved_stdin, saved_stdout);
        } else if (strchr(command, ';') != NULL) {
            handle_sequential_command(command);
        } else if (strchr(command, '+') != NULL) {
            handle_background_command(command);
        } else if (strcmp(command, "fore") == 0) {
            handle_foreground_command();
        } else {
            // General command execution
            char *args[MAX_ARG_COUNT];
            int argc;
            split_command(command, args, &argc);
            execute_command(args, 0);
        }
    }

    // Remove the current PID from the PID file before exiting
    remove_pid_from_file(my_pid);
    return 0;
}
