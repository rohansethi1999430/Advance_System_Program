#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define MAX_FILE_COUNT 1000
#define MAX_FILE_NAME_LENGTH 512

char **fileNames = NULL;
int fileCount = 0;

// Function declarations
int list_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int string_ends_with(const char *s, const char *suffix);
void move_file(const char *old_path, const char *new_path);
void copy_file(const char *old_path, const char *new_path);
char *get_full_pathname(const char *dirPath, const char *filename);
const char *get_file_name(const char *filePath);
int check_directory(const char *dirPath);
void exit_the_program(int status);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -flag directory_path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *flag = argv[1];
    char *dirPath = argv[2];

    printf("Flag: %s\n", flag);
    printf("Received directory path: %s\n", dirPath);

    if (!check_directory(dirPath)) {
        printf("Invalid or inaccessible directory.\n");
        exit(EXIT_FAILURE);
    }

    // Handling different flags
    if (strcmp(flag, "-nf") == 0) {
        // Process directory and list files
        if (nftw(dirPath, list_files, 20, FTW_PHYS) == -1) {
            perror("Error Reading the directory");
            exit_the_program(1);
        }

        printf("Total files: %d\n", fileCount);
    } else if (strcmp(flag, "-mv") == 0 || strcmp(flag, "-cp") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Usage for moving or copying: %s %s source_dir destination_dir filename\n", argv[0], flag);
            exit(EXIT_FAILURE);
        }
        char *sourceDir = argv[2];
        char *destinationDir = argv[3];
        char *filename = argv[4];

        if (!check_directory(sourceDir) || !check_directory(destinationDir)) {
            printf("Source or destination directory is invalid.\n");
            exit(EXIT_FAILURE);
        }

        char *fullPath = get_full_pathname(sourceDir, filename);
        char *destPath = get_full_pathname(destinationDir, filename);

        if (strcmp(flag, "-mv") == 0) {
            move_file(fullPath, destPath);
        } else {
            copy_file(fullPath, destPath);
        }

        free(fullPath);
        free(destPath);
    } else {
        printf("Unsupported flag. Please use -nf, -mv, or -cp.\n");
    }

    exit_the_program(0);
}

int check_directory(const char *dirPath) {
    printf("Attempting to open directory: %s\n", dirPath);
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        printf("Error opening directory: %s\n", strerror(errno));
        return 0;
    } else {
        closedir(dir);
        return 1;
    }
}

char *get_full_pathname(const char *dirPath, const char *filename) {
    char *temp = malloc(MAX_FILE_NAME_LENGTH);
    if (temp == NULL) {
        perror("Memory allocation failed");
        exit_the_program(1);
    }
    strcpy(temp, dirPath);
    if (temp[strlen(temp) - 1] != '/') {
        strcat(temp, "/");
    }
    strcat(temp, filename);
    return temp;
}

void move_file(const char *oldPath, const char *newPath) {
    if (rename(oldPath, newPath) == 0) {
        printf("File moved successfully.\n");
    } else {
        perror("Error moving file");
        exit_the_program(1);
    }
}

void copy_file(const char *oldPath, const char *newPath) {
    int fd1 = open(oldPath, O_RDONLY);
    if (fd1 == -1) {
        perror("Error opening source file");
        exit_the_program(1);
    }
    off_t file_size = lseek(fd1, 0, SEEK_END);
    char *file_content = malloc(file_size);
    if (file_content == NULL) {
        perror("Memory allocation failed");
        close(fd1);
        exit_the_program(1);
    }
    lseek(fd1, 0, SEEK_SET);
    read(fd1, file_content, file_size);
    close(fd1);

    int fd2 = open(newPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 == -1) {
        perror("Error opening destination file");
        free(file_content);
        exit_the_program(1);
    }
    write(fd2, file_content, file_size);
    close(fd2);
    free(file_content);
}

const char *get_file_name(const char *filePath) {
    const char *fileName = strrchr(filePath, '/');
    return fileName ? fileName + 1 : filePath;
}

int list_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        fileCount++;
    }
    return 0;
}

void exit_the_program(int status) {
    free(fileNames);
    exit(status);
}
