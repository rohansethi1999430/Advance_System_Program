#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>


// Global variable to manage the background process
pid_t last_bg_pid = -1; // PID of the last background process


// Process the command by identifying its type and handle accordingly
void handle_command(char *input) {
    char *pipe_check = strchr(input, '|');
    char *seq_check = strchr(input, ';');
    char *and_check = strstr(input, "&&");
    char *or_check = strstr(input, "||");
    char *word_count_check = strchr(input, '#');
    char *concat_check = strchr(input, '~');
    char *background_check = strchr(input, '+');

    // Determine the type of command based on special characters
    int command_type = 0;
    if (pipe_check) command_type = 1;
    else if (seq_check) command_type = 2;
    else if (and_check || or_check) command_type = 3;
    else if (word_count_check && input[0] == '#') command_type = 4;
    else if (concat_check) command_type = 5;
    else if (background_check) command_type = 6;

    switch (command_type) {
        case 1:
            handle_pipe(input);
            break;
        case 2:
            handle_sequence(input);
            break;
        case 3:
            handle_conditional(input);
            break;
        case 4:
            count_words(input + 1);
            break;
        case 5:
            concat_files(input);
            break;
        case 6: {
            // Extract command and arguments using a different approach
            char *args[5] = { NULL };
            char *token = strtok(input, "+");
            int count = 0;

            if (token != NULL) {
                char *cmd_token = strtok(token, " ");
                while (cmd_token != NULL && count < 4) {
                    args[count++] = cmd_token;
                    cmd_token = strtok(NULL, " ");
                }
            }

            if (count > 0) {
                execute_command(args, 1); // Execute in background
            }
            break;
        }
        default: {
            // Handle regular commands with a different token extraction method
            char *args[5] = { NULL };
            char *token = strtok(input, " ");
            int count = 0;

            while (token != NULL && count < 4) {
                args[count++] = token;
                token = strtok(NULL, " ");
            }

            if (count > 0) {
                execute_command(args, 0); // Execute in foreground
            }
            break;
        }
    }
}


// Execute a command with optional background execution
void execute_command(char **args, int background) {
    pid_t pid = fork(); // Create a child process

    if (pid < 0) {
        // Error occurred during fork
        perror("Failed to create process");
    } else if (pid == 0) {
        // Child process executes this block
        redirect_io(args); // Handle any I/O redirection

        // Set the process group ID for background processes
        if (background) {
            setpgid(0, 0); // Set child process group ID to its own PID
        }

        execvp(args[0], args); // Execute the command
        perror("Command execution failed"); // Print error if execvp fails
        exit(EXIT_FAILURE); // Exit if execvp fails
    } else {
        // Parent process executes this block
        if (!background) {
            // Wait for child process to finish if not running in background
            waitpid(pid, NULL, 0);
        } else {
            last_bg_pid = pid; // Store the background process PID
            printf("Background process started with PID: %d\n", pid);
        }
    }
}

// Bring the last background process to the foreground
void bring_to_foreground() {
    if (last_bg_pid > 0) {
        printf("Moving process %d to foreground\n", last_bg_pid);
        tcsetpgrp(STDIN_FILENO, last_bg_pid); // Set foreground process group for terminal
        kill(last_bg_pid, SIGCONT); // Continue the background process if it was stopped
        waitpid(last_bg_pid, NULL, WUNTRACED); // Wait for the background process to finish
        tcsetpgrp(STDIN_FILENO, getpid()); // Restore terminal control to the shell
        last_bg_pid = -1; // Reset the background PID
    } else {
        printf("No background process to move to foreground\n");
    }
}

// Handle input/output redirection
void redirect_io(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            // Input redirection
            if (args[i + 1] == NULL) {
                printf("Error: No input file specified after '<'\n");
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("Unable to open file for input");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO); // Redirect standard input
            close(fd);
            args[i] = NULL; // Remove redirection operator and file name from args
        } else if (strcmp(args[i], ">") == 0) {
            // Output redirection (overwrite)
            if (args[i + 1] == NULL) {
                printf("Error: No output file specified after '>'\n");
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Unable to open file for output");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Redirect standard output
            close(fd);
            args[i] = NULL; // Remove redirection operator and file name from args
        } else if (strcmp(args[i], ">>") == 0) {
            // Output redirection (append)
            if (args[i + 1] == NULL) {
                printf("Error: No output file specified after '>>'\n");
                exit(EXIT_FAILURE);
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("Unable to open file for appending");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Redirect standard output
            close(fd);
            args[i] = NULL; // Remove redirection operator and file name from args
        } else {
            // Check for redirection operators without spaces and notify the user
            if (strstr(args[i], ">") && !strstr(args[i], ">>")) {
                printf("Error: Redirection operator '>' should be surrounded by spaces.\n");
                exit(EXIT_FAILURE);
            } else if (strstr(args[i], ">>")) {
                printf("Error: Redirection operator '>>' should be surrounded by spaces.\n");
                exit(EXIT_FAILURE);
            } else if (strstr(args[i], "<")) {
                printf("Error: Redirection operator '<' should be surrounded by spaces.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

// Handle commands with pipes
void handle_pipe(char *input) {
    // Split the input based on the '|' character
    char *commands[5];
    int count = 0;
    char *token = strtok(input, "|");

    // Extract individual commands
    while (token != NULL && count < 4) {
        commands[count++] = token;
        token = strtok(NULL, "|");
    }

    // Create pipes
    int pipefds[8]; // File descriptors for up to 4 pipes
    for (int i = 0; i < (count - 1) * 2; i += 2) {
        if (pipe(pipefds + i) < 0) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < count; i++) {
        pid_t pid = fork(); // Create a child process for each command

        if (pid < 0) {
            perror("Failed to create process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process executes this block
            if (i != 0) {
                // Redirect input from previous pipe
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            if (i != count - 1) {
                // Redirect output to next pipe
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }

            // Close all pipe file descriptors
            for (int j = 0; j < 2 * (count - 1); j++) {
                close(pipefds[j]);
            }

            // Split command into arguments
            char *args[5];
            int cmd_count = 0;
            char *cmd_token = strtok(commands[i], " ");

            // Extract command and arguments
            while (cmd_token != NULL && cmd_count < 4) {
                args[cmd_count++] = cmd_token;
                cmd_token = strtok(NULL, " ");
            }

            args[cmd_count] = NULL;

            // Execute the command
            if (execvp(args[0], args) == -1) {
                perror("Failed to execute command");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close all pipe file descriptors in the parent
    for (int i = 0; i < 2 * (count - 1); i++) {
        close(pipefds[i]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < count; i++) {
        wait(NULL);
    }
}

void handle_sequence(char *input) {
    char *commands[5]; // Assuming max 5 commands
    int cmd_index = 0;

    char *cmd_start = input;
    char *cmd_end;

    // Split commands based on ';'
    while ((cmd_end = strchr(cmd_start, ';')) != NULL) {
        *cmd_end = '\0'; // Replace ';' with null terminator
        commands[cmd_index++] = cmd_start;
        cmd_start = cmd_end + 1; // Move to next command
    }
    commands[cmd_index++] = cmd_start; // Last command

    // Execute each command
    for (int i = 0; i < cmd_index; i++) {
        char *cmd = commands[i];

        // Trim leading and trailing spaces
        while (*cmd == ' ') {
            cmd++;
        }
        char *end = cmd + strlen(cmd) - 1;
        while (end > cmd && *end == ' ') {
            *end-- = '\0';
        }

        if (strlen(cmd) > 0) {
            // Split command into arguments
            char *args[5]; // Assuming max 5 arguments
            int arg_index = 0;

            char *arg = strtok(cmd, " ");
            while (arg != NULL && arg_index < 4) {
                args[arg_index++] = arg;
                arg = strtok(NULL, " ");
            }
            args[arg_index] = NULL; // Null terminate arguments array

            // Execute the command
            if (arg_index > 0) {
                execute_command(args, 0); // Assuming not running in background
            }
        }
    }
}

// Handle conditional commands with '&&' and '||'
void handle_conditional(char *input) {
    // Split the input based on '&&' and '||' characters
    char *commands[5];
    char *delims = "&&||";
    char *token = strtok(input, delims);
    int count = 0;

    // Extract individual commands
    while (token != NULL && count < 4) {
        commands[count++] = token;
        token = strtok(NULL, delims);
    }

    int prev_success = 1; // Track the success of the previous command
    for (int i = 0; i < count; i++) {
        // Skip commands based on the success of the previous command and logical operator
        if ((i > 0 && strstr(input, "&&") && !prev_success) ||
            (i > 0 && strstr(input, "||") && prev_success)) {
            continue;
        }

        pid_t pid = fork(); // Create a child process

        if (pid < 0) {
            perror("Failed to create process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process executes this block
            char *args[5];
            char *cmd_token = strtok(commands[i], " ");
            int cmd_count = 0;

            // Extract command and arguments
            while (cmd_token != NULL && cmd_count < 4) {
                args[cmd_count++] = cmd_token;
                cmd_token = strtok(NULL, " ");
            }

            args[cmd_count] = NULL;
            execvp(args[0], args); // Execute the command
            perror("Failed to execute command");
            exit(EXIT_FAILURE); // Exit if execvp fails
        } else {
            // Parent process executes this block
            int status;
            waitpid(pid, &status, 0); // Wait for child process to finish
            prev_success = WIFEXITED(status) && WEXITSTATUS(status) == 0; // Update success status
        }
    }
}

void count_words(char *filename) {
    // Trim leading spaces from filename
    while (*filename == ' ') {
        filename++;
    }

    // Open the file for reading
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Unable to open file for reading");
        return;
    }

    int words = 0;
    char buffer[1024];

    // Read each line from the file
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *token = strtok(buffer, " \t\n"); // Split by space, tab, or newline
        while (token != NULL) {
            words++;
            token = strtok(NULL, " \t\n");
        }
    }

    fclose(file);
    printf("Total words in file: %d\n", words);
}

void concat_files(char *input) {
    // Split the input based on the '~' character
    char *files[5];
    int count = 0;
    char *token = strtok(input, "~");

    // Handle leading and trailing spaces for each file name
    while (token != NULL && count < 4) {
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') end--;
        *(end + 1) = '\0';
        files[count++] = token;
        token = strtok(NULL, "~");
    }

    // Open a temporary file for writing concatenated content
    FILE *temp_file = tmpfile();
    if (!temp_file) {
        perror("Unable to create temporary file");
        return;
    }

    for (int i = 0; i < count; i++) {
        // Open each file for reading
        FILE *file = fopen(files[i], "r");
        if (!file) {
            fprintf(stderr, "Error: Unable to open file '%s' for reading.\n", files[i]);
            continue;
        }

        char buffer[1024];
        // Read and write file content to temporary file
        while (fgets(buffer, sizeof(buffer), file)) {
            fputs(buffer, temp_file);
        }

        fclose(file);
    }

    // Reset the position in the temporary file to the beginning
    rewind(temp_file);

    // Display the concatenated content from the temporary file
    char display_buffer[1024];
    while (fgets(display_buffer, sizeof(display_buffer), temp_file)) {
        fputs(display_buffer, stdout);
    }

    // Close and delete the temporary file
    fclose(temp_file);
}

int main() {
    char input[1024];

    while (1) {
        // Display command prompt
        printf("minibash$ ");
        
        // Read user input from stdin
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Failed to read input");
            continue;
        }

        // Remove newline character from input
        input[strcspn(input, "\n")] = '\0';

        // Check for the exit command 'dter'
        if (strcmp(input, "dter") == 0) {
            break;
        }

        // Check for the 'fore' command
        if (strcmp(input, "fore") == 0) {
            bring_to_foreground();
            continue;
        }

        // Process the user input
        handle_command(input);
    }

    return 0;
}