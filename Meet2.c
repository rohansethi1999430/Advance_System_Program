#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

/**
 * 
 * Meet Narendrakumar Patel
 * Assignment 2
 * ASP
 * 110126852
 * 
*/

#define MAX_COMMAND_LENGTH 256
#define MAX_OUTPUT_LENGTH 1024
#define MAX_PATH_LENGTH 100
#define LINE_BUFFER_SIZE 20

int operation_flag = 0;

// Function to get the parent process ID (PPID) given the process ID (PID)
// Returns the parent process ID (PPID) on success, or 0 if there was an error or no PPID found
int get_parent_id(pid_t process_id) 
{
    FILE *fp;

    char path[MAX_PATH_LENGTH];

    // Construct the path to the status file of the process
    sprintf(path, "/proc/%d/status", process_id);

    fp = fopen(path, "r");

    if (fp == NULL)
    {
        return -1;
    }

    int ppid;

    char line[LINE_BUFFER_SIZE];

    while (fgets(line, sizeof(line), fp)) 
    {
        // Check if the line contains the PPid entry
        if (sscanf(line, "PPid:\t%d", &ppid) == 1) 
        {
            fclose(fp);

            return ppid;        
        }
    }

    fclose(fp);

    // Return -1 if no PPID found or an error occurred
    return -1;
}

// Function to check if a process belongs to the same process tree as a given root process
int is_same_process_tree(pid_t root_process, pid_t process_id) 
{
    int parent_pid = get_parent_id(process_id);
    
    // Check if the parent process ID matches the root process ID
    if (parent_pid == root_process)
    {
        return 1;
    }
    else if (parent_pid == -1)
    {
        return 0;
    }
    // If not, recursively check the parent process
    else
    {
        return is_same_process_tree(root_process, parent_pid);
    }
}

// Function to kill a process by sending a signal
void kill_process(pid_t process_id, int signal) 
{
    if (kill(process_id, signal) == -1) 
    {
        perror("Kill");
        exit(EXIT_FAILURE);
    }
}

// Function to pause a process with a given PID
// Writes the PID to a file named "pause_process" and sends SIGSTOP signal to the process
// Returns 0 on success, or -1 if there was an error
int pause_process(pid_t pid) 
{
    char command[100];
    
    snprintf(command, sizeof(command), "echo %d >> %s", pid, "./pause_process");

    // Execute the shell command
    int result = system(command);

    if (result == -1) 
    {
        perror("Error executing command");
        return -1;
    } 
    else if (result != 0) 
    {
        printf("Command failed with exit code %d\n", result);
        return -1;
    }

    // Send SIGSTOP signal to the process
    if (kill(pid, SIGSTOP) == -1) 
    {
        perror("Kill");
        exit(EXIT_FAILURE);
    }

    return 0;
}

// Function to continue paused processes
// Reads process IDs from the "pause_process" file and sends SIGCONT signal to each process
// Returns 0 on success, or non-zero if there was an error
int continue_process() 
{
    FILE *fp;

    // Open the pause_process file for reading
    fp = fopen("./pause_process", "r");

    if (fp == NULL)
    {
        return 0;
    }

    char line[20];
    int pid;

    while (fgets(line, sizeof(line), fp))
    {
        if (sscanf(line, "%d", &pid) == 1)
        {
            // Send SIGCONT signal to the process
            if (kill(pid, SIGCONT) == 0) 
            {
                operation_flag = 1;

                printf("SIGCONT signal sent successfully to PID %d\n", pid);
            } 
            else
            {
                perror("Error sending SIGCONT signal");

                return 1;
            }
        }
    }

    fclose(fp);

    // Remove the pause_process file
    remove("./pause_process");

    return 0;
}

// Function to check if a process with the given PID is a zombie process
// Returns 1 if the process is a zombie, 0 otherwise or if an error occurred
int is_zombie(pid_t process_id) 
{
    FILE *fp;

    char path[100];

    sprintf(path, "/proc/%d/status", process_id);

    fp = fopen(path, "r");

    if (fp == NULL)
    {
        return 0;
    }

    char state[3];

    char line[20];

    while (fgets(line, sizeof(line), fp))
    {
        // Check if the line contains the State entry
        if (sscanf(line, "State:\t%s", state) == 1) 
        {
            fclose(fp);

            if (strcmp(state, "Z") == 0)
            {
                return 1; 
            }
            else 
            {
                return 0; 
            }
        }
    }

    fclose(fp);

    return 0; 
}

// Function to recursively find children processes of a given parent process
// Parameters:
//  - parent_pid: PID of the parent process
//  - options: option to specify the behavior of the function
//  - siblings: PID of a sibling process to be ignored
void find_children(pid_t parent_pid,int options, int siblings) 
{
    DIR *dir;

    struct dirent *entry;

    // Open the /proc directory
    dir = opendir("/proc");

    if (dir == NULL) 
    {
        perror("opendir");
        return;
    }

    // Read each entry in the /proc directory
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_type == DT_DIR) 
        {
            // Check if the entry name is a number (i.e., process ID)
            pid_t pid = atoi(entry->d_name);

            if (pid != 0 && pid != parent_pid && pid != siblings) 
            {
                char filename[100];
                snprintf(filename, sizeof(filename), "/proc/%d/stat", pid);

                FILE *fp = fopen(filename, "r");
                if (fp != NULL) 
                {
                    int ppid;
                    char state;
                    fscanf(fp, "%*d %*s %c %d", &state,&ppid);
                    fclose(fp);

                    if (ppid == parent_pid) 
                    {
                        if(options == 0)
                        {
                            //immediate descendants of process_id and sibling processes of process_id
                            printf("%d\n", pid);

                            operation_flag = 1;
                        }
                        else if (options == 1)
                        {
                            printf("%d\n", pid);

                            operation_flag = 1;

                            find_children(pid,1, 0);
                        }
                        else if (options == 2)
                        {
                            //lists the PIDs of all the non-direct descendants of process_id
                            find_children(pid,1,0);
                        }
                        else if(options == 3)
                        {
                            //lists the PIDs of all the grandchildren of process_id 
                            find_children(pid,0,0);
                        }
                        else if(options == 4)
                        {
                            // Lists the PIDs of all descendents of process_id that are defunct
                            if(is_zombie(pid)==1)
                            {
                                printf("%d\n", pid);

                                operation_flag = 1;
                            }
                            find_children(pid,4,0);
                        }
                    }
                }
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) 
{
    if (argc < 3) 
    {
        fprintf(stderr, "Usage: %s [process_id] [root_process] [OPTION]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t process_id = atoi(argv[1]);
    pid_t root_process = atoi(argv[2]);

    if (!is_same_process_tree(root_process, process_id)) 
    {
        printf("Does not belong to the process tree\n");
        return EXIT_SUCCESS;
    }

    if (argc == 4) 
    {
        char* option = argv[3];
        if (strcmp(option, "-rp") == 0) 
        {
            //process_id is killed if it belongs to the process tree rooted at root_process
            kill_process(process_id, SIGKILL);
        }
        else if (strcmp(option, "-pr") == 0)
        {
            //the root_process is killed (if it is valid) 
            kill_process(root_process, SIGKILL);
        }
        else if (strcmp(option, "-xd") == 0)
        {
            //immediate descendants of process_id
            find_children(process_id,0,0);

            if(operation_flag == 0)
            {
                printf("No direct descendants\n");
            }
        }
        else if (strcmp(option, "-xn") == 0)
        {
            //lists the PIDs of all the non-direct descendants of process_id
            find_children(process_id,2,0);

            if(operation_flag == 0)
            {
                printf("No non-direct descendants\n");
            }
        }
        else if (strcmp(option, "-xs") == 0) 
        {
            //sibling processes of process_id 
            find_children(get_parent_id(process_id),0,process_id);

            if(operation_flag == 0)
            {
                printf("No sibling/s\n");
            }
        }
        else if (strcmp(option, "-xg") == 0)
        {
            //lists the PIDs of all the grandchildren of process_id 
            find_children(process_id,3,0);

            if(operation_flag == 0)
            {
                printf("No grandchildren of process_id\n");
            }
        }
        else if (strcmp(option, "-xt") == 0) 
        {
            //process_id is paused with SIGSTOP
            if(pause_process(process_id) != 0)
            {
                printf("paused with SIGSTOP Failed\n");
            }
        }
        else if (strcmp(option, "-xc") == 0)
        {
            // Send SIGCONT to all processes paused by root_process
            continue_process();

            if(operation_flag == 0)
            {
                printf("No process is paused\n");
            }
        }
        else if (strcmp(option, "-xz") == 0)
        {
            // Lists the PIDs of all descendents of process_id that are defunct
            find_children(process_id,4,0);

            if(operation_flag == 0)
            {
                printf("No descendents are defunct\n");
            }
        }
        else if (strcmp(option, "-zs") == 0)
        {
            // Print status of process_id (Defunct/Not Defunct)
            if (is_zombie(process_id))
            {
                printf("Defunct\n");
            }
            else
            {
                printf("Not Defunct\n");
            }
        }
        else
        {
            fprintf(stderr, "Invalid option\n");
            return EXIT_FAILURE;
        }
    }
    else 
    {
        printf("PID: %d, PPID: %d\n", process_id, get_parent_id(process_id));
    }

    return EXIT_SUCCESS;
}