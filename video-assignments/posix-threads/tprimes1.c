//veniamin velikoretskikh 
//CS333 video assignment 4 threads

//to compile
//gcc -g3 -o0 -Wall -Werror -o tprimes1 tprimes1.c

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <stdbool.h>

#define OPTIONS "u:h"

static long upper_bound = 0;

static void * find_primes(long);

int
main(int argc, char *argv[])
{
	{
		int opt = 0;
		while (( opt = getopt(argc, argv, OPTIONS)) != -1){
			switch (opt) {
			case 'u':
			{
				int ret = sscanf(optarg, "%ld", &upper_bound);
		
				if(ret != 1){
					fprintf(stderr, "invalid value for upper bound %s\n", optarg);
					exit(EXIT_FAILURE);
				}	
			}	
			break;
			case 'h':
			default:
				printf("Usage: %s %s\n", argv[0], OPTIONS);
				printf("\t-u ub     upperbound\n");
				printf("\t-h 	    help\n");
				exit(EXIT_SUCCESS);
			}
		}
	}

	if(upper_bound < 0){
		fprintf(stderr, "invalid value for upper bound %ld\n", upper_bound);
		exit(EXIT_FAILURE);
	}	

	find_primes(0);

	return EXIT_SUCCESS;
}


static void *
find_primes(long begin)
{
	long candidate = begin;
	long found = 0;

	while (candidate < upper_bound){
		bool is_prime = true;
		
		if(candidate > 1) {
			for(long factor = 2; factor < candidate; ++ factor){
				if((candidate % factor) == 0){
					is_prime = false;
					break;
				}
			}
			
			if(is_prime){
				printf("%ld\n", candidate);
				found ++;
			}
		}
		candidate +=1;
	}

	fprintf(stderr, "%2ld %8ld\n", begin, found);

	return NULL;
}




