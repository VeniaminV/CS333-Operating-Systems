// Veniamin Velikoretskikh veniamin@pdx.edu
// CS333 Lab4 - monitorasaurus
// System monitor using mutex timedlock for periodic display refresh


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
 
#include "monitorasaurus.h"

// Global flag set by signal handler to stop the main loop
volatile sig_atomic_t keep_running = true;

 
static void handle_sigint(int sig);
static void print_help(const char *prog);

int
main(int argc, char *argv[])
{
	//This is the getopt() option character
	int opt;

	// Command line option values
	int interval  = DEFAULT_INTERVAL; //interval between display
	int count     = INFINITE_COUNT;  //how many refreshes befor exiting
	int max_width = DEFAULT_COLS;    //max num of char per line
	int verbose   = 0;		//for diagnostic messages

	// Tracks number of screen refreshes
	int iter = 0;

	// Return code from pthread timed lock
	int rc;

	struct sigaction sa; // struct to register handle_sigint for SIGINT 
	pthread_mutex_t mtx; //mutex block
	struct timespec ts;  // holds teh future timeout time for the mutes time lock

	FILE *fp;	//stream returned by popen(), to read the command output

	// Buffer for reading command output for popen()
	char line[MAX_LINE_LEN + 2];

        // Process command line options using getopt()
        while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {

	// Refresh interval in seconds
        case 'i':
        	interval = atoi(optarg);

            	// Clamp interval to valid range
            	if (interval < MIN_INTERVAL) {
                	if (verbose) {
                   	fprintf(stderr,
                            "Interval %d too small; using %d\n",
                            interval,
                            MIN_INTERVAL);
                }

                interval = MIN_INTERVAL;
           	}

           	if (interval > MAX_INTERVAL) {
                	if (verbose) {
                    	fprintf(stderr,
                            	"Interval %d too large; using %d\n",
                            	interval,
                            	MAX_INTERVAL);
                	}

                	interval = MAX_INTERVAL;
            	}

            	break;

            // Number of display refreshes before exiting
         case 'c':
         	count = atoi(optarg); //converts the command line arg from string to int
         	// Negative counts are invalid
         	if (count < 0) {
             	if (verbose) {
                 	fprintf(stderr,
                         	"Invalid count; using fallback %d\n",
                         	FALLBACK_COUNT);
             	}
	
             	count = FALLBACK_COUNT;
         	}

         	break;

        // Maximum width of displayed text
	case 'w':
		max_width = atoi(optarg);

		// Restore default width if invalid
		if (max_width <= 0) {
			max_width = DEFAULT_COLS;
        	}

        	break;

        // Enable verbose logging to stderr
        case 'v':
            	verbose = 1;
            	break;

        // Print help message and exit
        case 'h':
            	print_help(argv[0]);
            	break;

        default:
            	fprintf(stderr,
                    	"Unknown option. Use -h for help.\n");

            	exit(EXIT_FAILURE);
        }
	}

        // Display option settings in verbose mode
    	if (verbose) {
        	fprintf(stderr,
                	"interval=%d  count=%d  max_width=%d\n",
                	interval, count, max_width);
	}
	
	//This is going to be the SIGINT handler so pressing Ctrl-C will safely terminate
	//memset zeros the struct to avoid undefined behaviour
    	memset(&sa, 0, sizeof(sa));
	//sets keep_running to false, so look exits cleanly
    	sa.sa_handler = handle_sigint;

    	// Initialize signal mask to empty
    	sigemptyset(&sa.sa_mask);
	//flags 0, uses default behaviour
    	sa.sa_flags = 0;

    	if (sigaction(SIGINT, &sa, NULL) == -1) {
        	perror("sigaction");
        	exit(EXIT_FAILURE);
    	}


	// Initialize the mutex and immediately lock it.
    	// The mutex is intentionally never unlocked.
    	// Each call to pthread_mutex_timedlock() will block
    	// until the timeout expires, which acts as our periodic timer.
    	mtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    	if (pthread_mutex_lock(&mtx) != 0) {
        	perror("pthread_mutex_lock");
        	exit(EXIT_FAILURE);
    	}



	//the main monitoring loop 
	while(keep_running){
		//stop if refresh count limit is reached
		if(count != INFINITE_COUNT && iter >= count){
			break;
		}

		//because mutes thread lock needs the future time.
		//get present adn add the delay
		if(clock_gettime(CLOCK_REALTIME, &ts) == -1){
			perror("clock_gettime");
			break;
		}

		ts.tv_sec += interval;

		// Wait until the timeout expires.
        	// Since the mutex is permanently locked, the call should always
        	// return ETIMEDOUT unless interrupted by a signal like Ctrl-C.
        	rc = pthread_mutex_timedlock(&mtx, &ts);

        	// This should never happen because the mutex is already locked
        	if (rc == 0) {
            	if (verbose) {
                	fprintf(stderr,
                        	"Unexpected mutex acquisition.\n");
            	}

            	break;
        	}

		// EINTR means the wait was interrupted by a signal
        	if (rc == EINTR) {
            	if (verbose) {
                	fprintf(stderr,	"Interrupted by signal.\n");
            	}

            	break;
        	}

        	// ETIMEDOUT is the normal expected result
        	if (rc != ETIMEDOUT) {
            	if (verbose) {
                	fprintf(stderr,
                        	"pthread_mutex_timedlock unexpected error: %d\n",
                        	rc);
            	}

            	break;
        	}


		//clear the screen and us popen()
		printf(CLEAR_SCREEN);

		fp = popen(CMD, "r");

		if(fp == NULL){
			perror("popen");
			break;
		}
	
		

		//reading form the commad output
		//if line is to long, truncate it
        	while (fgets(line, (int)sizeof(line), fp) != NULL) {

            		if ((int)strlen(line) > max_width)
			{
                		// Replace character at max_width with newline
                		line[max_width] = '\n';
                		// Properly terminate the string
                		line[max_width + 1] = '\0';
            		}

            		// Output processed line to the terminal
            		fputs(line, stdout);
        	}

        	//pclose() waits for child process to finish and then frees memory	
		pclose(fp);

        	// Increment refresh counter
        	++iter;

        	// Verbose status update
        	if (verbose) {
            	fprintf(stderr,	"Refresh #%d\n", iter);
        	}
    	}

    	// Destroy mutex before exiting
    	pthread_mutex_destroy(&mtx);

    	return EXIT_SUCCESS;
}


// Signal handler for Ctrl-C (SIGINT)
static void
handle_sigint(int sig)
{
    	(void)sig;

    	keep_running = false;
}


// Print usage information and exit
static void
print_help(const char *prog)
{
    	printf("Usage: %s [%s]\n", prog, OPTIONS);

    	printf("  -i #   Set refresh interval in seconds "
           	"(default: %d, min: %d, max: %d)\n",
           	DEFAULT_INTERVAL,
           	MIN_INTERVAL,
           	MAX_INTERVAL);

    	printf("  -c #   Number of refreshes before exiting "
           	"(default: run until Ctrl-C)\n");

    	printf("  -w #   Maximum width of displayed text "
           	"(default: %d)\n",
           	DEFAULT_COLS);

    	printf("  -v     Enable verbose logging to stderr\n");

    	printf("  -h     Print this help and exit\n");

    	exit(EXIT_SUCCESS);
}





