# include <stdio.h> 

int main(void)
{ 

int *a; //a is a pointer variable to an integer 
int b=100;
a=&b; // pointer variable a is assigned the address of b
*a=200; //dereferencing 

printf("\nThe address of b is %d", &b);
printf("\nThe value of a is %d", a);

printf("\nThe value of b is %d", *a);

} 
