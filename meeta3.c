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

int num_bg_processes = 0;
pid_t bg_process_pids[MAX_BG_PROCESSES];


// Function to tokenize a string based on a delimiter and count the number of tokens
char **tokenize(const char *command, const char *delimiter, int *num_tokens)
{
    // Allocating memory for an array of character pointers
    char **tokens = (char **)malloc(MAX_TOKENS * sizeof(char *));
    if (tokens == NULL)
    {
        // Error handling for memory allocation failure
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Using strtok to split the command string into tokens
    char *token = strtok((char *)command, delimiter);
    int count = 0;
    while (token != NULL && count < MAX_TOKENS)
    {
        // Allocating memory for each token and storing it in the tokens array
        tokens[count] = strdup(token);
        if (tokens[count] == NULL)
        {
            // Error handling for memory allocation failure
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        count++;
        token = strtok(NULL, delimiter);
    }

    // Storing the number of tokens in the num_tokens pointer
    *num_tokens = count;
    return tokens;
}

// Function to free memory allocated for tokens
void free_tokens(char **tokens, int num_tokens)
{
    // Freeing memory for each token
    for (int i = 0; i < num_tokens; i++)
    {
        free(tokens[i]);
    }

    // Freeing memory for the tokens array
    free(tokens);
}


// Function to count the number of tokens in a string based on a delimiter
int count_tokens(const char *command, const char *delimiter)
{
    // Copying the command string to avoid modifying the original
    char *command_copy = strdup(command);
    if (command_copy == NULL)
    {
        // Error handling for memory allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    int count = 0;
    char *token = strtok(command_copy, delimiter);

    // Counting tokens using strtok
    while (token != NULL)
    {
        count++;
        token = strtok(NULL, delimiter);
    }

    // Freeing the memory allocated for the copied command string
    free(command_copy); 
    return count;
}

// Function to check the presence of pipe symbols in a string
int check_pipe_presence(const char *command)
{
    const char *ptr = command;
    while (*ptr)
    {
        if (*ptr == '|')
        {
            if (*(ptr + 1) == '|')
            {
                return 1; // "||" found
            }
            else
            {
                return 0; // "|" found
            }
        }
        ptr++;
    }
    return -1; // No pipe found
}

// Function to execute a command
int execute_command(const char *command) 
{
    // Array to store command and its arguments
    char *args[MAX_COMMAND_LENGTH];
    int arg_count = 0;
    char *token = strtok((char *)command, " ");

    // Parsing command string into tokens
    while (token != NULL && arg_count < MAX_COMMAND_LENGTH - 1) 
    {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL; // Null-terminating the argument array

    // Executing the command using execvp
    if (execvp(args[0], args) == -1) 
    {        
        perror("execvp");
        return 0; // Return 0 on failure
    }

    return 1; // Return 1 on success
}

// Function to fork a process and execute a command
int fork_and_execute_command(const char *command) {
    pid_t pid;

    pid = fork();

    if (pid == -1) 
    {
        // Error handling for fork failure
        perror("fork");
        return 0; // Return 0 on failure
    }

    if (pid == 0) 
    {
        // Child process: execute the command
        execute_command(command);
        
        return 0; // Return 0 to indicate execution in child process
    }
    else 
    {
        // Parent process: wait for child to finish
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
        {
            return 1; // Return 1 on successful execution
        }
        else 
        {
            return 0; // Return 0 on failure
        }
    }
}

// Function to execute multiple commands separated by semicolons
void multiple_commands(char* list_of_commands)
{
    int n;
    char** commands = tokenize(list_of_commands, ";", &n);

    if(n<=5)
    {
        // Loop through and execute each command
        for (int i = 0; i < n; i++)
        {
            fork_and_execute_command(commands[i]);
        }
    }
    else
    {
        printf("Only 5 operations are permitted\n");
    }
}

// Function to run a command in background
void run_in_background(char* command)
{
    pid_t pid = fork();
    num_bg_processes++;

    if (pid < 0)
    {
        // Error handling for fork failure
        printf("fork error");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Child process: execute the command
        execute_command(command);
        printf("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process: print background process information
        printf("Background process started: %s\n", command);
        bg_process_pids[num_bg_processes - 1] = pid;
    }
}

// Function to bring a background process to foreground
void bring_to_foreground(char* bg_processes) 
{
    if (num_bg_processes > 0) 
    {
        printf("Process:%s\n", bg_processes);

        int status;

        // Waiting for the specified background process to finish
        waitpid(bg_process_pids[num_bg_processes - 1], &status, 0);
    }
    else 
    {
        printf("Invalid background process index\n");
    }
}

// Function to execute commands in a pipe
void execute_commands_in_pipe(char** commands, int num_pipes) 
{
    int pipes[num_pipes - 1][2];

    // Creating pipes for communication between processes
    for (int v = 0; v < num_pipes - 1; v++)
    {
        if (pipe(pipes[v]) < 0)
        {
            printf("pipe error");
            exit(EXIT_FAILURE);
        }
    }

    // Checking if the number of parameters in each command exceeds the limit
    for (int i = 0; i < num_pipes; i++)
    {
        if (count_tokens(commands[i], " ") > 5)
        {
            printf("Only 5 parameters are permitted\n");
            return;
        }
    }
    
    // Forking child processes to execute each command in the pipe
    for (int v = 0; v < num_pipes; v++)
    {
        pid_t proid = fork();
        if (proid < 0)
        {
            printf("fork error");
            exit(EXIT_FAILURE);
        }
        else if (proid == 0)
        {
            // Setting up file descriptors for input and output redirection
            if (v != 0)
            {
                dup2(pipes[v - 1][0], STDIN_FILENO);
                close(pipes[v - 1][0]);
            }
            if (v != num_pipes - 1)
            {
                dup2(pipes[v][1], STDOUT_FILENO);
                close(pipes[v][1]);
            }

            // Closing unnecessary pipe ends
            for (int j = 0; j < num_pipes - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Executing the command
            execute_command(commands[v]);
        }
    }

    // Closing all pipe ends in the parent process
    for (int v = 0; v < num_pipes - 1; v++)
    {
        close(pipes[v][0]);
        close(pipes[v][1]);
    }

    // Waiting for all child processes to finish
    for (int v = 0; v < num_pipes; v++)
    {
        wait(NULL);
    }
}

// Function to concatenate strings separated by "#" and remove "#" symbols
char *concatenate_with_spaces_remove_hash(const char *input) 
{
    char *output = NULL;
    char *token;
    char *delim = "#";
    size_t output_size = 5;

    char *input_copy = strdup(input);

    // Memory allocation for output string
    output = (char *)malloc(output_size);
    if (output == NULL) 
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    strcpy(output, "cat "); // Starting with "cat" command

    // Tokenizing input string and concatenating tokens with spaces
    token = strtok(input_copy, delim);
    while (token != NULL) 
    {
        output = realloc(output, output_size + strlen(token) + 1);
        if (output == NULL) 
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcat(output, token);
        strcat(output, " ");
        output_size += strlen(token) + 1;

        token = strtok(NULL, delim);
    }

    // Trimming the trailing space
    if (output_size > 1) 
    {
        output[output_size - 2] = '\0';
    }

    free(input_copy);

    return output;
}

// Function to execute a command and redirect its output/input to a file
void execute_and_redirect(const char *command, const char *filename, const char *mode) 
{
    // Forking a child process
    pid_t pid = fork();

    if (pid == -1) 
    {
        // Error handling for fork failure
        printf("Error forking!\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) 
    {
        // Child process: redirecting stdout to the specified file
        freopen(filename, mode, stdout);
        
        // Executing the command
        execute_command(command);
    } 
    else 
    {
        // Parent process: waiting for the child process to finish
        int status;
        waitpid(pid, &status, 0);
    }
}

// Function to remove leading and trailing whitespaces from a string
char *trimString(char *str) 
{
    // Removing leading whitespaces
    while (isspace((unsigned char)*str)) 
    {
        str++;
    }

    // Returning if the string is empty
    if (*str == '\0') {
        return str;
    }

    // Removing trailing whitespaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    // Null terminating the string
    *(end + 1) = '\0';

    return str;
}

// Function to handle redirection (output/input redirection)
void redirect(const char *command)
{
    int n;

    // Checking for output redirection (append)
    if(strstr(command, ">>"))
    {
        char ** tokens = tokenize(command, ">>", &n);
        execute_and_redirect(tokens[0], trimString(tokens[1]), "a");
    }
    // Checking for output redirection (overwrite)
    else if(strstr(command, ">"))
    {
        char ** tokens = tokenize(command, ">", &n);
        execute_and_redirect(tokens[0], trimString(tokens[1]), "w");
    }
    // Checking for input redirection
    else if(strstr(command, "<"))
    {
        char ** tokens = tokenize(command, "<", &n);
        execute_and_redirect(tokens[1], trimString(tokens[0]), "w");
    }
}

// Signal handler function
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

// Function to count logical operators (&& and ||) in a string
int count_logical_operators(const char *input_string) 
{
    int and_count = 0; 
    int or_count = 0;  

    // Iterate through the input string
    for (int i = 0; input_string[i] != '\0'; i++) 
    {    
        // Count && operator pairs
        if (input_string[i] == '&' && input_string[i+1] == '&') 
        {
            and_count++;
            i++; // Skip the next character since it's part of the operator
        }
        // Count || operator pairs
        else if (input_string[i] == '|' && input_string[i+1] == '|') 
        {
            or_count++;
            i++; // Skip the next character since it's part of the operator
        }
    }
    
    return and_count + or_count; // Return the total count of logical operators
}

int main()
{
    char command[MAX_COMMAND_LENGTH]; // Buffer for user input

    // Register signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, handler);

    char *bg_processes[10]; // Array to store background processes

    while (1)
    {
        printf("Shell24$ "); // Prompt for user input

        fgets(command, MAX_COMMAND_LENGTH, stdin); // Read user input from stdin

        command[strcspn(command, "\n")] = 0; // Remove trailing newline character

        if (strlen(command) > 0)
        {
            // Check for built-in commands or special operations
            
            if (strcmp(command, "newt") == 0)
            {
                // Launch a new terminal window with the same shell
                if (fork() == 0)
                {
                    execlp("xterm", "xterm", "-e", "./shell24", NULL); // Execute in child process
                    perror("execlp"); // Print error if execlp fails
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(command, "exit") == 0)
            {
                return 0; // Exit the shell if "exit" command is entered
            }
            else
            {
                // Check for comments
                if(strchr(command,'#'))
                {
                    // Process commands with comments
                    if(count_tokens(command,"#") <= 5)
                    {
                        fork_and_execute_command(concatenate_with_spaces_remove_hash(command)); // Execute command
                    }
                    else
                    {
                        printf("Only 5 operations are permitted\n"); // Print error message
                    }
                    continue;
                }
                // Check for redirection operators
                if (strstr(command, ">") || strstr(command, ">>") || strstr(command, "<"))
                {
                    redirect(command); // Redirect command input/output
                    continue;
                }
                // Check for multiple commands separated by semicolons
                if(strstr(command, ";"))
                {
                    multiple_commands(command); // Execute multiple commands
                    continue;
                }
                // Check for logical operators (&& and ||)
                if(strstr(command, "&&") || strstr(command, "||")) 
                {
                    if(count_logical_operators(command) >= 5) 
                    {
                        printf("Up to 5 conditional execution operators are supported\n"); // Print error message
                        continue;
                    }
                    // Execute commands with logical operators
                    int n1, n2, flag=0;
                    char** and_tokens = tokenize(command, "&&", &n1);
                    char** or_tokens;

                    for (int i = 0; i < n1; i++) 
                    {
                        if(strstr(and_tokens[i], "||")) 
                        {
                            or_tokens = tokenize(and_tokens[i], "||", &n2);

                            for (int j = (flag==0? 0:1); j < n2; j++) 
                            {
                                if(fork_and_execute_command(or_tokens[j]) == 1) 
                                {
                                    break;
                                }
                            }

                            flag = 0;
                            
                            free_tokens(or_tokens, n2);
                        }
                        else if(flag == 0) 
                        {
                            if(fork_and_execute_command(and_tokens[i]) != 1) 
                            {
                                flag = 1;
                            }
                        }
                    }
                    
                    free_tokens(and_tokens, n1);
                    continue;
                }
                // Check for background execution
                if(command[strlen(command) - 1] == '&')
                {
                    command[strlen(command) - 1] = '\0'; // Remove '&' from the command
                    bg_processes[num_bg_processes] = strdup(command); // Store background process
                    run_in_background(command); // Execute command in background
                    continue;
                }
                // Check for bringing background process to foreground
                if (strcmp(command, "fg") == 0)
                {
                    bring_to_foreground(bg_processes[num_bg_processes - 1]); // Bring background process to foreground
                    continue;
                }
                // Check for pipe presence
                if (check_pipe_presence(command) == 0)
                {
                    int n;
                    char **tokens = tokenize(command, "|", &n); // Tokenize the command by pipe symbol
                    if (n > 7)
                    {
                        printf("Only 6 Pipes are permitted\n"); // Print error message
                        continue;
                    }
                    execute_commands_in_pipe(tokens, n); // Execute commands in pipe
                    free_tokens(tokens, n); // Free memory allocated for tokens
                    continue;
                }
                // Check for parameter limit
                if (count_tokens(command, " ") > 5)
                {
                    printf("Only 5 parameters are permitted\n"); // Print error message
                    continue;
                }
                // Execute command
                fork_and_execute_command(command);
            }
        }
    }

    return 0;
}
