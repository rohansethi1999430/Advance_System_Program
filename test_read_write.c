#define _POSIX_SOURSE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <fcntl.h>
#include<string.h>
// #include<sys/types.h>
 #include<sys/stat.h>

void wrt(int fd, const char *text){
    if(write(fd,text,strlen(text))==-1){

        perror("\n error in writing to the file");
        close(fd);
        exit(0);
    }
}

int main(){

  umask(0000);

  int fd = open("check.txt",O_CREAT|O_RDWR,0700);

  wrt(fd,"COMP 8567");
  wrt(fd,"COMP 8567");
  close(fd);
  
  

}