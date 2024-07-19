        #include <stdio.h>
        #include <stdlib.h>
        #include <string.h>
        #include <dirent.h>
        #include <unistd.h>
        #include <signal.h>
        #include <ctype.h>
        #include <sys/types.h>
        #include <errno.h>

        // Function declarations
        void walk_process_tree(pid_t parent, void (*func)(pid_t));
        void terminate_descendants(pid_t parent);
        int check_if_defunct(pid_t proc);
        void display_grandchildren(pid_t proc);
        void signal_descendants(pid_t parent, int sig);
        void display_siblings(pid_t proc, int defunct_only);
        void list_all_descendants(pid_t parent, int direct_only, int defunct_only, int orphan_only);
        void report_error(const char *msg);
        void stop_proc(pid_t proc);
        void show_direct_descendants(pid_t parent);
        int check_if_orphan(pid_t proc);
        void terminate_proc(int proc);
        void send_signal_to_tree(pid_t parent, int sig);
        pid_t get_ppid_of_proc(pid_t proc);
        void list_non_direct_descendants_recur(pid_t parent, pid_t orig_parent);
        int check_if_descendant(pid_t parent, pid_t proc);
        void terminate_action(pid_t proc);
        void show_proc_status(pid_t proc, int type);
        void verify_process_tree(pid_t root, pid_t proc);
        void continue_proc(pid_t proc);
        void terminate_parents_of_defuncts(pid_t parent);
        int fetch_ppid(pid_t proc);
        void show_non_direct_descendants(pid_t parent);
        void verify_process_tree_optionless(pid_t root, pid_t proc);

        // Function to get the parent process ID from /proc/[proc]/stat
        pid_t get_ppid_of_proc(pid_t proc) {
            char file[256];
            snprintf(file, sizeof(file), "/proc/%d/stat", proc); // Construct the file path
            FILE *fp = fopen(file, "r"); // Open the file
            if (fp == NULL) {
                return -1; // Return -1 if the process directory doesn't exist
            }
            int dummy;
            char comm[256], state;
            pid_t ppid;
            if (fscanf(fp, "%d %s %c %d", &dummy, comm, &state, &ppid) != 4) {
                fclose(fp);
                return -1; // Return -1 if the fscanf fails
            }
            fclose(fp);
            return ppid; // Return the parent process ID
        }

        // Function to check if a process is defunct (zombie)
        int check_if_defunct(pid_t proc) {
            char file[256];
            char line[256];
            int defunct = 0;
            snprintf(file, sizeof(file), "/proc/%d/status", proc); // Construct the file path
            FILE *fp = fopen(file, "r"); // Open the file
            if (fp == NULL) {
                return 0; // Return 0 if the process directory doesn't exist
            }
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "State:", 6) == 0) {
                    if (strstr(line, "Z")) {
                        defunct = 1; // Mark as defunct if the state is 'Z'
                        break;
                    }
                }
            }
            fclose(fp);
            return defunct; // Return whether the process is defunct
        }

        // Function to check if a process is an orphan
        int check_if_orphan(pid_t proc) {
            pid_t ppid = get_ppid_of_proc(proc);
            return (ppid == 1); // Return 1 if the parent process ID is 1 (init process)
        }

        // Error handling function
        void report_error(const char *msg) {
            perror(msg); // Print the error message
            exit(EXIT_FAILURE); // Exit the program
        }

        // Get the parent process ID using /proc/[proc]/stat
        int fetch_ppid(pid_t proc) {
            char path[256];
            snprintf(path, sizeof(path), "/proc/%d/stat", proc); // Construct the file path
            FILE *fp = fopen(path, "r"); // Open the file
            if (!fp) {
                return -1; // Return -1 if the file doesn't exist
            }
            int ppid;
            fscanf(fp, "%*d %*s %*c %d", &ppid); // Read the parent process ID
            fclose(fp);
            return ppid; // Return the parent process ID
        }

        // Function to check if a process is a descendant of another
        int check_if_descendant(pid_t parent, pid_t proc) {
            while (proc != 1 && proc != parent) {
                proc = get_ppid_of_proc(proc); // Fetch the parent process ID
                if (proc == -1) {
                    return 0; // Return 0 if the parent process ID is invalid
                }
            }
            return proc == parent; // Return whether the process is a descendant of the parent
        }

        // List the PID and PPID of a process
        void display_pid_ppid(pid_t parent, pid_t proc) {
            if (!check_if_descendant(parent, proc)) {
                printf("The process %d does not belong to the tree rooted at %d\n", proc, parent);
                return;
            }
            int ppid = fetch_ppid(proc);
            if (ppid != -1) {
                printf("PID: %d, PPID: %d\n", proc, ppid); // Print the process ID and parent process ID
            }
        }

        // Action to kill a process
        void terminate_action(pid_t proc) {
            if (kill(proc, SIGKILL) == -1) {
                report_error("kill"); // Report an error if the kill fails
            }
        }

        // Action to stop a process
        void stop_proc(pid_t proc) {
            if (kill(proc, SIGSTOP) == -1) {
                report_error("kill"); // Report an error if the stop fails
            }
        }

        // Action to stop a process, excluding the root
        void stop_proc_except_root(pid_t proc, pid_t root) {
            if (proc != root) {
                stop_proc(proc);
            }
        }

        // Action to continue a process
        void continue_proc(pid_t proc) {
            if (kill(proc, SIGCONT) == -1) {
                report_error("kill"); // Report an error if the continue fails
            }
        }

        // Action to continue a process, excluding the root
        void continue_proc_except_root(pid_t proc, pid_t root) {
            if (proc != root) {
                continue_proc(proc);
            }
        }

        // Traverse the process tree and perform an action on each process
        void walk_process_tree(pid_t parent, void (*func)(pid_t)) {
            DIR *dir = opendir("/proc"); // Open the /proc directory
            if (!dir) {
                report_error("opendir"); // Report an error if the directory can't be opened
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (isdigit(entry->d_name[0])) {
                    pid_t proc = atoi(entry->d_name); // Convert the directory name to a process ID
                    if (check_if_descendant(parent, proc)) {
                        func(proc); // Perform the action on the process
                    }
                }
            }
            closedir(dir);
        }

        // Traverse the process tree and perform an action on each process, excluding the root
        void walk_process_tree_except_root(pid_t parent, pid_t root, void (*func)(pid_t, pid_t)) {
            DIR *dir = opendir("/proc"); // Open the /proc directory
            if (!dir) {
                report_error("opendir"); // Report an error if the directory can't be opened
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (isdigit(entry->d_name[0])) {
                    pid_t proc = atoi(entry->d_name); // Convert the directory name to a process ID
                    if (check_if_descendant(parent, proc)) {
                        func(proc, root); // Perform the action on the process
                    }
                }
            }
            closedir(dir);
        }

        // Kill all descendants of a process
        void terminate_descendants(pid_t parent) {
            walk_process_tree(parent, terminate_action); // Traverse and terminate all descendants
        }

        // Function to kill a specific process
        void terminate_proc(int proc) {
            if (kill(proc, SIGKILL) == -1) {
                printf("Failed to kill process %d: %s\n", proc, strerror(errno)); // Print an error message
            } else {
                printf("Process %d killed.\n", proc); // Print a success message
            }
        }

        // Send a specific signal to all descendants of a process
        void send_signal_to_tree(pid_t parent, int sig) {
            sig == SIGKILL ? terminate_descendants(parent) : 
            sig == SIGSTOP ? walk_process_tree(parent, stop_proc) : 
            sig == SIGCONT ? walk_process_tree(parent, continue_proc) : (void)0;
        }

        // Send a specific signal to all descendants of a process, excluding the root
        void send_signal_to_tree_except_root(pid_t parent, int sig, pid_t root) {
            sig == SIGKILL ? terminate_descendants(parent) : 
            sig == SIGSTOP ? walk_process_tree_except_root(parent, root, stop_proc_except_root) : 
            sig == SIGCONT ? walk_process_tree_except_root(parent, root, continue_proc_except_root) : (void)0;
        }

        // Function to send a signal to all descendants of a process
        void signal_descendants(pid_t parent_pid, int signal) {
            send_signal_to_tree(parent_pid, signal); // Send the signal to all descendants
        }

        // Function to send a signal to all descendants of a process, excluding the root
        void signal_descendants_except_root(pid_t parent_pid, int signal, pid_t root) {
            send_signal_to_tree_except_root(parent_pid, signal, root); // Send the signal to all descendants except root
        }

        // Kill the parents of all zombie processes that are descendants of root
        void terminate_parents_of_defuncts(pid_t parent) {
            DIR *dir = opendir("/proc"); // Open the /proc directory
            if (!dir) {
                report_error("opendir"); // Report an error if the directory can't be opened
            }
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (isdigit(entry->d_name[0])) {
                    pid_t proc = atoi(entry->d_name); // Convert the directory name to a process ID
                    if (check_if_descendant(parent, proc) && check_if_defunct(proc)) {
                        pid_t ppid = get_ppid_of_proc(proc); // Get the parent process ID
                        if (ppid > 1) {
                            printf("Killing parent process %d of zombie process %d\n", ppid, proc);
                            terminate_proc(ppid); // Terminate the parent process
                        }
                    }
                }
            }
            closedir(dir);
        }

        // Function to list all descendants of a process
        void list_all_descendants(pid_t parent_pid, int direct_only, int defunct_only, int orphan_only) {
            DIR *proc_dir;
            struct dirent *entry;

            if ((proc_dir = opendir("/proc")) == NULL) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            int found = 0;
            while ((entry = readdir(proc_dir)) != NULL) {
                if (entry->d_type == DT_DIR) {
                    pid_t proc = atoi(entry->d_name);
                    if (proc > 0) {
                        pid_t ppid = get_ppid_of_proc(proc);
                        if (ppid == parent_pid) {
                            if ((defunct_only && check_if_defunct(proc)) || (orphan_only && check_if_orphan(proc)) || (!defunct_only && !orphan_only)) {
                                printf("%d\n", proc);
                                found = 1;
                            }
                            if (!direct_only) {
                                list_all_descendants(proc, 0, defunct_only, orphan_only);
                            }
                        }
                    }
                }
            }

            closedir(proc_dir);

            if (!found && direct_only) {
                printf("No descendants found\n");
            }
        }

        // Function to list all immediate descendants of a process
        void show_direct_descendants(pid_t parent_pid) {
            list_all_descendants(parent_pid, 1, 0, 0);
        }

        // Helper function to list all non-direct descendants of a process
        void list_non_direct_descendants_recur(pid_t parent_pid, pid_t orig_parent) {
            DIR *proc_dir;
            struct dirent *entry;

            if ((proc_dir = opendir("/proc")) == NULL) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            while ((entry = readdir(proc_dir)) != NULL) {
                if (entry->d_type == DT_DIR) {
                    pid_t proc = atoi(entry->d_name);
                    if (proc > 0) {
                        pid_t ppid = get_ppid_of_proc(proc);
                        if (ppid == parent_pid) {
                            list_non_direct_descendants_recur(proc, orig_parent);
                            if (ppid != orig_parent) {
                                printf("%d\n", proc);
                            }
                        }
                    }
                }
            }

            closedir(proc_dir);
        }

        // Function to list all non-direct descendants of a process
        void show_non_direct_descendants(pid_t parent_pid) {
            list_non_direct_descendants_recur(parent_pid, parent_pid);
        }

        // Function to list siblings of a process
        void display_siblings(pid_t proc, int defunct_only) {
            pid_t ppid = get_ppid_of_proc(proc);
            DIR *proc_dir;
            struct dirent *entry;

            if ((proc_dir = opendir("/proc")) == NULL) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            int found = 0;
            while ((entry = readdir(proc_dir)) != NULL) {
                if (entry->d_type == DT_DIR) {
                    pid_t pid = atoi(entry->d_name);
                    if (pid > 0 && pid != proc) {
                        pid_t parent_pid = get_ppid_of_proc(pid);
                        if (parent_pid == ppid && (!defunct_only || check_if_defunct(pid))) {
                            printf("%d\n", pid);
                            found = 1;
                        }
                    }
                }
            }

            if (!found) {
                printf("No siblings found\n");
            }

            closedir(proc_dir);
        }

        // Function to list grandchildren of a process
        void display_grandchildren(pid_t proc) {
            DIR *proc_dir;
            struct dirent *entry;

            if ((proc_dir = opendir("/proc")) == NULL) {
                perror("opendir");
                exit(EXIT_FAILURE);
            }

            int found = 0;
            while ((entry = readdir(proc_dir)) != NULL) {
                if (entry->d_type == DT_DIR) {
                    pid_t pid = atoi(entry->d_name);
                    if (pid > 0) {
                        pid_t ppid = get_ppid_of_proc(pid);
                        if (ppid == proc) {
                            show_direct_descendants(pid);
                            found = 1;
                        }
                    }
                }
            }

            if (!found) {
                printf("No grandchildren found\n");
            }

            closedir(proc_dir);
        }

        // Function to print the status of a process
        void show_proc_status(pid_t proc, int type) {
            if (type == 0) { // Check if process is defunct
                if (check_if_defunct(proc)) {
                    printf("Defunct\n");
                } else {
                    printf("Not defunct\n");
                }
            } else if (type == 1) { // Check if process is orphan
                if (check_if_orphan(proc)) {
                    printf("Orphan\n");
                } else {
                    printf("Not orphan\n");
                }
            }
        }

        // Verify and print process tree rooted at 'root' and containing 'proc'
        void verify_process_tree(pid_t root, pid_t proc) {
            pid_t current = proc;
            pid_t ppid = get_ppid_of_proc(current);

            if (ppid == -1) {
                printf("Process %d does not belong to the tree rooted at %d\n", proc, root);
                return; // Process doesn't exist
            }

            // Track the hierarchy from proc to root
            struct {
                pid_t pid;
                pid_t ppid;
            } hierarchy[256];
            int count = 0;

            while (current != root && current != 1 && ppid != -1) {
                hierarchy[count].pid = current;
                hierarchy[count].ppid = ppid;
                count++;
                current = ppid;
                ppid = get_ppid_of_proc(current);
            }

            if (current == root) {
                printf("Process tree rooted at %d includes:\n", root);
                for (int i = count - 1; i >= 0; i--) {
                    printf("PID: %d, PPID: %d\n", hierarchy[i].pid, hierarchy[i].ppid);
                }
            } else {
                printf("Process %d does not belong to the tree rooted at %d\n", proc, root);
            }
        }

        // Verify and print process tree rooted at 'root' and containing 'proc' (option-less)
        void verify_process_tree_optionless(pid_t root, pid_t proc) {
            if (!check_if_descendant(root, proc)) {
                printf("The process %d does not belong to the tree rooted at %d\n", proc, root);
                return;
            }
            display_pid_ppid(root, proc);
        }

        // Function to process command line options
        int processOptions(char *option, pid_t root, pid_t proc) {
            if (strcmp(option, "-gc") == 0) {
                display_grandchildren(proc);
            } else if (strcmp(option, "-dx") == 0) {
                terminate_descendants(root);
            } else if (strcmp(option, "-bz") == 0) {
                display_siblings(proc, 1);
            } else if (strcmp(option, "-dt") == 0) {
                signal_descendants_except_root(root, SIGSTOP, root);
            } else if (strcmp(option, "-so") == 0) {
                show_proc_status(proc, 1);
            } else if (strcmp(option, "-sz") == 0) {
                show_proc_status(proc, 0);
            } else if (strcmp(option, "-kz") == 0) {
                terminate_parents_of_defuncts(root);
            } else if (strcmp(option, "-dc") == 0) {
                signal_descendants_except_root(root, SIGCONT, root);
            } else if (strcmp(option, "-rp") == 0) {
                terminate_proc(proc);
            } else if (strcmp(option, "-sb") == 0) {
                display_siblings(proc, 0);
            } else if (strcmp(option, "-od") == 0) {
                list_all_descendants(proc, 0, 0, 1);
            } else if (strcmp(option, "-nd") == 0) {
                show_non_direct_descendants(proc);
            } else if (strcmp(option, "-dd") == 0) {
                show_direct_descendants(proc);
            } else if (strcmp(option, "-zd") == 0) {
                list_all_descendants(proc, 0, 1, 0);
            } else {
                printf("Invalid option provided\n");
            }
        }

        // Main function to handle command line arguments and call appropriate functions
        int main(int argc, char *argv[]) {
            if (argc < 3) {
                printf("You entered wrong number of arguments please recheck..!!");
                exit(EXIT_FAILURE);
            }

            if (argc == 3) {
                pid_t root = (pid_t)atoi(argv[1]);
                pid_t proc = (pid_t)atoi(argv[2]);
                verify_process_tree_optionless(root, proc);
            } else {
                pid_t root = (pid_t)atoi(argv[2]);
                pid_t proc = (pid_t)atoi(argv[3]);
                char *option = argv[1];
                processOptions(option, root, proc);
            }

            return 0;
        }
