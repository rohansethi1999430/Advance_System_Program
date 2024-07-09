#include <stdio.h>

int main(void){
    int a  = 10, b;
    int * ptr;

    ptr = &a;

    b = *ptr;

    printf("value of a = %d value of b = %d value of *ptr = %d",a,b,*ptr);
}