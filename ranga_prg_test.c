//Writes an array of characters into check.txt
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include<stdlib.h>
int main(void)
{
int fd = open("check.txt",O_CREAT|O_RDWR,0700);
write(fd,"test123",7);

write(fd,"test123",8);

int long n = lseek(fd,5,SEEK_SET);
printf("\n N = %d",n);
char *buff = "test99";
n = write(fd,buff,6);
printf("\n N = %d",n);

lseek(fd,50,SEEK_SET);
write(fd,buff,6);
lseek(fd,0,SEEK_SET);
    int c = 0;
    char buffer[1];
    while(read(fd,buffer,1)>0){
        if(buffer[0]=='\0'){
            lseek(fd,-1,SEEK_CUR);
            write(fd,"$",1);
        }
        c++;
    }

    // read(fd,fileContent,fileSize);

    // printf("Print file content");
    // printf("\nFile: %s", fileContent);

    //     for(int i = 0;i<fileSize;i++){
    //     printf("%s",fileContent[i]);
    // }
    

    //     for (int i = 0; i < fileSize; i++) {
    //     if (fileContent[i] == NULL) {
    //         //printf("I = %d",i);
    //         fileContent[i] = '$';
    //     }
    // }
    //write(fd,fileContent,24);
    // for(int i = 0;i<fileSize;i++){
    //     printf("%c",fileContent[i]);
    // }
    

// for(int i =0;i<24;i++){
//     if(charAt(i)=='\0'){
//         chatAt(i) = '$';
//     }
// }

// while()

}
