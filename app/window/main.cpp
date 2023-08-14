#include<stdio.h>

//#include"staticlibtest.h"
//#include"dynamiclibtest.h"
#include<staticlibtest.h>
#include<dynamiclibtest.h>

int main()
{
	printf("Hello, RHI");
	staticlibtest();
	dynamiclibtest();
	return 0;
}