#include<stdio.h>
#include<stdlib.h>
#include<limits.h>


void * rev_array(int *ar,int size){

    printf("Reverse array: ");
    for (int i =size-1;i>-1;i--){
        printf("%d ",*(ar+i));
    }
}


int main(void){

    int size=0;

    
    int x;
    printf("\nEnter the size of the array: ");
    scanf("%d",&size);

    printf("\nSize: %d",size);

    int *ar = malloc(size*sizeof(int));

    printf("\nEnter %d Elements seprated by space ", size);
    for(int i = 0;i<size;i++){
        scanf("%d",& *(ar+i));
        
    }

    //     for( int i =0;i<size;i++){
    //     scanf("%d",&ar[i]);
    // }

    printf("\nInput array: ");
    for (int i = 0 ;i<size ; i++){
        printf("%d ",*(ar+i));
    }

    printf("\n Calling the reverse function: ");

    rev_array(ar,size);


}