#include <stdio.h>
#include<limits.h>
#include<stdlib.h>
//creating the function to calculate the max of array
int calMax(int *ar,int size){
    int max = INT_MIN;
    int maxindex = 0;
    for(int i = 0;i<size;i++ ){
        if(max < *(ar+i)){
            // printf("Max element : ")
            max = *(ar+i);
            maxindex = i;
        }
    }
    return *(ar+maxindex);
}
int main(void){
    int size = 0;
//Taking the user input
    printf("Enter the Size of array: ");
    scanf("%d",&size);
// Dynamic memory allocation using malloc
    int *ar = malloc(size*sizeof(int));
    int max;
    printf("Enter %d elements with space in between ",size);
    //creating array without using array index and using pointers only[]
    for( int i =0;i<size;i++){
        scanf("%d",& *(ar+i));
    }
    max = calMax(ar,size);
    printf("\n The Max element is : %d  \n",max);

}