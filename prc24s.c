#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    printf("\nLength is %d ",argc);
    if (argc < 3) {
       printf("\nLength is %d ",argc);
        exit(EXIT_FAILURE);
    }
    else if(argc == 3){
        printf("\n NO OPTION PROVIDED!!!");
        
    }
}