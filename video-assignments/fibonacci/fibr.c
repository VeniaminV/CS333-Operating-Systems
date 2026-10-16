//Veniamin Velikoretskikh veniamin@pdx.edu
//Video assignment 1 finonacci Recursion

//gcc -Wall firb.c -o fibr

#include <stdio.h>
#include <stdlib.h>

unsigned long fib(unsigned long);

unsigned long
fib(unsigned long n)
{
	if(n <=1)
	{ return n; }
	
	return fib(n-1) + fib(n-2);
}

int
main(int argc, char *argv[])
{
	unsigned long n =0;

	if(argc < 2){
		printf("Must give value on command line\n");
		exit(EXIT_FAILURE);
	}

	n = atol(argv[1]);
	
	if(n > 93 || n <1){
		printf("Enter a number between 1 and 93 (inclusive)\n");
		exit(EXIT_FAILURE);
	}
	printf("%6d: %lu\n", 0, 0ul);
	printf("%6d: %lu\n", 1, 0ul);

	for (unsigned long i = 2; i <= n; i++)
	{
		printf("%6ld: %lu\n", i, fib(i));
	}

	return EXIT_SUCCESS;
}
	

