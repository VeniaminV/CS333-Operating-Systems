//Veniamin Velikoretskikh veniamin@pdx.edu
//Video assignment 1 finonacci Recursion

//gcc -Wall fibi.c -o fibi

#include <stdio.h>
#include <stdlib.h>

int 
main(int argc, char *argv[])
{
	long n = 0;
	unsigned long t1 = 0ul;
	unsigned long t2 = 1ul;
	unsigned long next_fib = t1 + t2;

	if(argc < 2){
		printf("must give value on command line/\n");
		exit(EXIT_FAILURE);		
	}
	n = strtol(argv[1], NULL, 10);

	if(n > 93 || n <1){
		printf("Enter a number between 1 and 93 (inclusive)\n");
		exit(EXIT_FAILURE);
	}

	printf("%6d: %lu\n", 0, t1);
	printf("%6d: %lu\n", 1, t2);

	for (long i = 2; i <= n; i++)
	{
		printf("%6ld: %lu\n", i, next_fib);
		t1= t2;
		t2 = next_fib;
		next_fib = t1 + t2;
	}


	return EXIT_SUCCESS;
}


