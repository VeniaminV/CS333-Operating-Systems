//veniamin velikoretskikh 
//CS333 video assignment 4 threads

//to compile
//gcc -g3 -o0 -Wall -Werror -o tprimes2 tprimes2.c

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <stdbool.h>

#define OPTIONS "u:h"
#define NS_PER_SECOND 1000000000.0
// this is a 1 adn 9 zeroes followed, by a decimal point and a zero

static long upper_bound = 0;

static void * find_primes(long);
static double elapse_time(struct timespec *, struct timespec *);

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
	struct timespec et0;
	struct timespec et1;
	double et = 0.0;
	
	clock_gettime(CLOCK_MONOTONIC, &et0);
	
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

	clock_gettime(CLOCK_MONOTONIC, &et1);

	et = elapse_time(&et0, &et1);


	fprintf(stderr, "%2ld %8ld et: %10.4lf\n", begin, found, et);

	return NULL;
}



static double
elapse_time(struct timespec *t0, struct timespec *t1)
{
	double et = (t1->tv_sec - t0->tv_sec)
		+ ((t1 -> tv_nsec - t0->tv_nsec) / NS_PER_SECOND);

	return et;
}
