#include <stdlib.h>

void fun(){
	int *p = (int *)malloc(10 * sizeof(int));
	p[10] = 0;
}

int main(){
fun();
return 0;
}
