#include <stdio.h>
int main(void) {
int num=100;
int *b;
b=&num; //Address of an integer variable 
printf("\nThe value of  *b is : %d\n",*b);

int **c; //Address of the (Address of an integer variable) 
c=&b;
printf("\nThe value of  **c is: %d\n",**c);

int ***d;//Address of the Address of the (Address of an integer variable )
d=&c;

printf("\nThe value of  *b is: %d\n",*b);
printf("\nThe value of  **c is: %d\n",**c);
printf("\nThe value of  ***d is: %d\n",***d);
}//End main
