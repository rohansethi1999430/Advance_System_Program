#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

void write_content(int fd, const char *content) 
{
    if (write(fd, content, strlen(content)) == -1) 
    {
        perror("Error writing to file");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

int main()
{
    umask(0000);

    /**
     * Task 1
     * Opening the file and adding the content
    */

    int fd=open("check.txt", O_CREAT|O_RDWR, 0700);

    write_content(fd,"COMP 8567");

    write_content(fd,"COMP 8567");

    close(fd);

    /**
     * Task 2
     * opening it in O_RDWR and adding the content at 50th Cur location
    */

    fd=open("check.txt", O_RDWR);

    if (lseek(fd, 50, SEEK_CUR) == -1) {
        perror("Error seeking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    write_content(fd,"COMP 8567");

    write_content(fd,"COMP 8567");

    close(fd);

    /**
     * Task 3
    */

    fd=open("check.txt", O_RDWR);

    // opening it in O_RDWR and adding the content at 12th Cur location from the end
    if (lseek(fd, 12, SEEK_END) == -1) {
        perror("Error seeking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    write_content(fd,"COMP 8567");

    // Getting back to the beginning and skipping the content "COMP 8567COMP 8567"
    if (lseek(fd, 18, SEEK_SET) == -1) {
        perror("Error seeking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 50 - 12 = 18
    // Replacing the null characters with the '$'
    for (int i = 0; i < 32; i++)
    {
        write_content(fd,"$");
    }

    // skipping the content "COMP 8567COMP 8567" from the cur location
    if (lseek(fd, 18, SEEK_CUR) == -1) {
        perror("Error seeking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // adding 12 '$' to replace null character because in task3 we had skipped 12 characters and added the content
    // for (int i = 0; i < 12; i++)
    // {
    //     write_content(fd,"$");
    // }

    close(fd);

    return 0;
}
