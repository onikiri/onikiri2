#include<stdio.h>
#define num 16
void main(){
	int a[num];
	int i;
	printf("address a : %p\naddress a : %p\naddress i : %p\n", &a,&a+num, &i);
	/*
	for(i=0; i<num;i++){
	 a[i] = i;
	 }
	 */
	for(i=0;i<num;i++){
		a[i] = i;
		printf("a[%d]  %d\n", i, a[i]);
	}
}

