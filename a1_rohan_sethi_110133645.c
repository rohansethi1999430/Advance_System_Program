#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h> 
#include <sys/stat.h>


//declating some global variables 
int fileCount = 0;//to count the number of files
int dirCount = 0;// to count the number of directories
char *source_dir;//source path provided by user
char *destination_dir;// destination path provided by user
char *exclude_extension = NULL;//to store the extention provided by user to be excluded

int copy_file(const char *src, const char *dst) {
    //opening the source file in read only mode
    int src_fd = open(src, O_RDONLY); // Open the source file for reading
    //checking if the source file is opened or not (using perror as it makes the error easier to understand)
    if (src_fd == -1) {
        perror("Error opening source file");
        return -1;
    }

    // creating the destination directory in Write only mode 
    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open the destination file for writing
    //cheking of the open was successfull or not 
    if (dst_fd == -1) {
        perror("Error opening destination file");
        return -1;


    }
    // if any of the file is not opened properly then we will return after closing both files
if (src_fd < 0) { // Check if source file failed to open
    perror("Error opening source file");
    return -1;
}

if (dst_fd < 0) { // Check if destination file failed to open
    perror("Error opening destination file");
    close(src_fd); // Close the source file descriptor if it was successfully opened
    return -1;
}

//declaring the string buff that will store the read content and the will help in pasting in the destination file
    char buff[4096];
    int bytes_read,bytes_written;
    //runnig the while loop as long as we are able to read from the source file and storing the read content in the string buffer
    while ((bytes_read = read(src_fd, buff, sizeof(buff))) > 0){
        //writing the content read in above line to the new destination file 
         bytes_written = write(dst_fd, buff, bytes_read);
         // if the length of content read and length of content written is not equal that means something went wrong, hence the below if condition
        if (bytes_written != bytes_read) {
            perror("write");
            close(src_fd);
            close(dst_fd);
            return -1;
        }
    }
//closing both the files after success
    close(src_fd);
    close(dst_fd);
    return 0;
}
//function for the copy path
int process_path_cpx(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char dst_path[4096];

    // Extract the base name of the source directory
    char *base_source_dir = basename(source_dir);

    // Correct the destination path to include the source directory's base name
strcpy(dst_path, destination_dir); 
strcat(dst_path, "/");             
strcat(dst_path, base_source_dir); 

// Conditionally append additional path components
if (strcmp(fpath, source_dir) == 0) {
    // If the current path is the root of the source directory
    // Nothing more needs to be appended since base_source_dir is already appended
} else {
    // Append the rest of the path that follows the source directory base path
    strcat(dst_path, fpath + strlen(source_dir));
}

    printf("\nDestination path for mkdir or file operation: %s\n", dst_path);

    if (typeflag == FTW_D) {
        if (mkdir(dst_path, sb->st_mode) < 0 && errno != EEXIST) {
            perror("mkdir failed");
            //printf("\nError occurred in mkdir, check if the directory already exists.");
            return -1;
        }
    } else if (typeflag == FTW_F) {
        // Process files
        printf("\nExtension: %s", strrchr(fpath, '.'));
        if (!exclude_extension || (strrchr(fpath, '.') && strcmp(exclude_extension, strrchr(fpath, '.')) != 0)) {
            if (copy_file(fpath, dst_path) < 0) {
                return -1;
            }
        }
    }

    return 0;
}



int process_path_mv(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char dst_path[4096];
    char src_base[1024];
    strcpy(src_base, source_dir);  // Make a copy to preserve the original source_dir
    char *base_source_dir = basename(src_base);

    // Construct the destination path by including the source directory's base name
    strcpy(dst_path, destination_dir);          
    strcat(dst_path, "/");                      
    strcat(dst_path, base_source_dir);          

    if (strcmp(fpath, source_dir) != 0) {
        // If not the root directory, append the relative file path
        strcat(dst_path, fpath + strlen(source_dir));
    }

    printf("\nConstructed destination path for move: %s\n", dst_path);


    if (typeflag == FTW_D) {
        // Create the directory if it doesn't exist, recursively creating parent directories as needed
        struct stat st = {0};
        if (stat(dst_path, &st) == -1) {
            mkdir(dst_path, 0755); // Create directory with default permissions
        }
    } else if (typeflag == FTW_F) {
        // Move files
        struct stat st;
        if (stat(dst_path, &st) == -1) {
            // Ensure the parent directory exists before moving a file
            char *dir_path = strdup(dst_path);
            mkdir(dirname(dir_path), 0755);
            free(dir_path);
        }

        if (rename(fpath, dst_path) != 0) {
            perror("Failed to move file");
            return -1;
        }
    }

    return 0;
}

//function to increament the fileCount when it excounters the FTW_F which is for type file
int list_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        fileCount++;
    }
    return 0;
}
//function to increament the dirCount when it excounters the FTW_D which is for type directory
int list_dir(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_D) {
        dirCount++;
    }
    return 0;
}

int remove_item(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int result = 0;

    // Check the type of file and remove it
    if (typeflag == FTW_DP || typeflag == FTW_D) {
        result = rmdir(fpath);
    } else {
        result = unlink(fpath);
    }

    // Check for errors
    if (result < 0) {
        perror(fpath);
    }

    return result;
}
//function to calidate the path provided by the user
int validateDir(const char* path) {
    printf("\nChecking the path provided: %s\n", path);

    //checking if we are able to open the file or not else we will tell user to check the path
    DIR *dir = opendir(path);

    if (dir == NULL) {
        printf("Please check the path provided.\n");
        return 0;
    }
//closing directory if path validation is success and we are able to open the directory
    closedir(dir);
    printf("Path validation success\n");
    return 1;
}
// this function will calculate the total size of all the files and also list them 
int calcTotalSize(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        char *filename = strrchr(fpath, '/');
        if (filename && *filename == '/') {
            filename++;
        }
        //printing the file name and the size of the file using st_size
        printf("File Name: %s \t Size: %lld bytes\n", filename, (long long)sb->st_size);
    }
    return 0;
}



int main(int argc, char* argv[]) {
    if (argc < 3) {
       // print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    

    char *command = argv[1];// command given by user
    char *dir_path = argv[2];// directory path provided by user

    printf("Command = %s\n", command);
    printf("Path = %s\n", dir_path);

    // if the command is -nf/-nd/-sf the we will first validate the source directory provided by the user
    if (strcmp(command, "-nf") == 0 || strcmp(command, "-nd") == 0 || strcmp(command, "-sf") == 0) {
        if (!validateDir(dir_path)) {
            //if validation failed the exit 
            exit(EXIT_FAILURE);
        }
    }
//for command -nf calling nftw function and passing the source directory path and list_files function which will update the counter variable for the number of files 
//using strcmp function to compare the string with command
    if (strcmp(command, "-nf") == 0) {
        if (nftw(dir_path, list_files, 20, FTW_PHYS) == -1) {
            //if this failes then displaying the error and exiting
            perror("Error reading the directory");
            exit(EXIT_FAILURE);
        }
        //printing the total count of fileCount variable 
        printf("Total files: %d\n", fileCount);
    } 
//if the command is -nd the calling nftw function with source directory and the list_dir function which updated the directory counter 
//using strcmp function to compare the string with command
    else if (strcmp(command, "-nd") == 0) {
        if (nftw(dir_path, list_dir, 20, FTW_PHYS) == -1) {
            // error in case of failure of nftw 
            perror("Error reading the directory");
            exit(EXIT_FAILURE);
        }
        //printing the firCount variable which is a global variable to store the count of directories
        printf("Total directories: %d\n", dirCount);
    } 
    else if (strcmp(command, "-sf") == 0) {
        if (nftw(dir_path, calcTotalSize, 20, FTW_PHYS) == -1) {
            perror("Error reading the directory");
            exit(EXIT_FAILURE);
        }
    } 
    //when command is -cpx
    else if (strcmp(command, "-cpx") == 0) {
        if (argc < 4 || argc > 5) {
            printf("\nPlease provide the valid input for copy");
            return EXIT_FAILURE;
        }

        source_dir = argv[2];
        destination_dir = argv[3];
        if (argc == 5) {
            exclude_extension = argv[4];
        }
//calling nftw function with source directory and process_path_cpx function whuch will handle the path construction for destination and also 
        if (nftw(source_dir, process_path_cpx, 20, 0) < 0) {
            perror("nftw");
            return EXIT_FAILURE;
        }

    }
    //when command is -mv
    else if(strcmp(command, "-mv") == 0){
            printf("\n Move command");
        source_dir = argv[2];
        destination_dir = argv[3];
        if(argc != 4 ){
            printf("\nPlease provide the valid input for move");
            return EXIT_FAILURE;
        }
        if (nftw(source_dir, process_path_mv, 64, FTW_DEPTH | FTW_PHYS) < 0) {
        perror("nftw");
        return -1;
    }
    //calling remove function which will remove the file once it has been copied to the destination
        if (nftw(source_dir, remove_item, 64, FTW_DEPTH | FTW_PHYS) < 0) {
        perror("nftw");
        return -1;
    }
    }
    
    else {
        //print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
