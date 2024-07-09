#include <stdio.h>
#include<limits.h>
#include<stdlib.h>


int * calMax(int *ar,int size){
    int max = INT_MIN;
    int maxindex = 0;
    for(int i = 0;i<size;i++ ){
        if(max < *(ar+i)){
            // printf("Max element : ")
            max = *(ar+i);
            maxindex = i;
        }
    }
    return ar+maxindex;
}

int main(void){
    int size = 0;
    printf("Enter the Size of array: ");
    scanf("%d",&size);

    int *ar = malloc(size*sizeof(int));
    int *maxadr;

    printf("Enter all the element with space in between ");

    for( int i =0;i<size;i++){
        scanf("%d",&ar[i]);
    }

    maxadr = calMax(ar,size);
    printf("\n The Max element address is : %d ",maxadr);

    printf("\n The Max element is : %d ",*maxadr);

}