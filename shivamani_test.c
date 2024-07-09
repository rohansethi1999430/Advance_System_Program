
#include<stdio.h>
#include<limits.h>

int main()
{


int i=0;
int arr[4]={10,20,360,40};


int *ptr=&i;
int *ptr_a=arr;
// int temp =0;
// int *temp_ptr=&temp;


int maxi=INT_MIN;
int *max = &maxi ;

for(*ptr=0;*ptr<4;(*ptr)++)

{

if((*max)< *(arr+(*ptr)))
{
*max = *(arr+(*ptr));

}


}


printf("\n%d\n",*max);


return 0;
}