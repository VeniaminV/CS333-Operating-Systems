//veniamin velikoretskikh 
//CS333 video assignment 4 threads

//to compile
//gcc -g3 -o0 -Wall -Werror -o tprimes3 tprimes3.c

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <stdbool.h>

#define OPTIONS "t:u:h"
#define MAX_THREADS 30
#define NS_PER_SECOND 1000000000.0
// this is a 1 adn 9 zeroes followed, by a decimal point and a zero

static long upper_bound = 0;
static int num_threads = 0;

static void * find_primes(void *arg);
static double elapse_time(struct timespec *, struct timespec *);

int
main(int argc, char *argv[])
{
	pthread_t *workers = NULL;
	{
		int opt = 0;
		while (( opt = getopt(argc, argv, OPTIONS)) != -1){
			switch (opt) {
			case 't':
			{
				int ret = sscanf(optarg, "%d", &num_threads);
				
				if(ret !=1 ){
					fprintf(stderr, "invali value for number of threads %s\n", optarg);
					exit(EXIT_FAILURE);
				}
			}
				break;
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
				printf("\t-t nt     number of threads\n");
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
	if((num_threads < 0) || (num_threads > MAX_THREADS)){
		fprintf(stderr, "invalid value for number of threads %d\n", num_threads);
		exit(EXIT_FAILURE);
	}

	workers = malloc (num_threads * (sizeof(pthread_t)));

	//find_primes(0);

	for(long tid = 0; tid < num_threads; ++tid){
		int ret = pthread_create(
			&(workers[tid])
			, NULL
			, find_primes
			, (void * ) tid
			);
		if(ret != 0){
			perror("thread creatino failed");
			fprintf(stderr, "thread creation failed on %ld\n", tid);
			exit(EXIT_FAILURE);
		}	
	}

	for(int tid = 0; tid < num_threads; ++tid){
		pthread_join(workers[tid], NULL);
	}
	
	free(workers);

	pthread_exit(NULL);
}


static void *
find_primes(void *arg)
{
	long begin = ((long) arg);
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
		candidate += num_threads;
	}

	clock_gettime(CLOCK_MONOTONIC, &et1);

	et = elapse_time(&et0, &et1);


	fprintf(stderr, "%2ld %8ld et: %10.4lf\n", begin, found, et);

	pthread_exit(NULL);
}



static double
elapse_time(struct timespec *t0, struct timespec *t1)
{
	double et = (t1->tv_sec - t0->tv_sec)
		+ ((t1 -> tv_nsec - t0->tv_nsec) / NS_PER_SECOND);

	return et;
}
