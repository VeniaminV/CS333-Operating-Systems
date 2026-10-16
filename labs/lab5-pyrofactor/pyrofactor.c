// Veniamin Velikoretskikh veniamin@pdx.edu
// CS 333 Lab 5 - Spring 2026
// pyrofactor.c - starts and manages worker processes that factor large integers
// workers communicate with pyrofactor through a shared memory segment

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include "pyrofactor.h"

// shared memory pointer and fd - need to be global so signal handlers
// and cleanup functions can access them
static factorize_data_t *shm_data = NULL;
static char *shm_name = NULL;
static int shm_fd = -1;

// verbose flag set by -v option
static int verbose = 0;

// these get set by signal handlers to break out of loops
// volatile so the compiler doesnt optimize away the checks
static volatile int got_sigint = 0;
static volatile int got_sigalrm = 0;


static void handle_sigint(int sig);
static void handle_sigterm(int sig);
static void handle_sigalrm(int sig);
static void print_help(const char *prog);
static double elapsed_sec(struct timeval *start);
static const char *alg_name(factor_algorithms a);
static factorize_data_t *create_shm(const char *name);
static void init_shm(factorize_data_t *data);
static void cleanup_shm(void);
static void remove_shm(void);
static void launch_naive(const char *seg_name, int worker_id, int n_naive);
static void launch_pollard(const char *seg_name, int worker_id);
static void launch_fermat(const char *seg_name, int worker_id, int n_fermat);
static void reap_all_children(void);


int
main(int argc, char *argv[])
{
	int opt;

	// command line options with defaults
	char *input_file = NULL;
	int timeout_min = 2;    // in minutes, gets multiplied by 60 later
	int n_naive   = 0;
	int n_pollard = 0;
	int n_fermat  = 0;
	int total_workers;
	int timeout_sec;
	int i;

	FILE *infile;

	// buffer to hold each number we read from the input file
	char line_buf[MAX_VALUE_LEN];

	// copies of the shared memory result fields so we can
	// print them after the workers are done
	bool found;
	int win_pid;
	int win_worker;
	factor_algorithms win_alg;
	char value[MAX_VALUE_LEN];
	char factor[MAX_VALUE_LEN];

	struct timeval tv_start;
	double elapsed;

	// used in the polling loop to sleep between checks
	struct timespec ts;

	// used to set up all three signal handlers
	struct sigaction sa;

	// process command line options using getopt()
	while ((opt = getopt(argc, argv, "n:i:t:N:P:F:vh")) != -1) {
	switch (opt) {

	// name of the shared memory segment, required
	case 'n':
		shm_name = optarg;
		break;

	// input file, defaults to stdin if not given
	case 'i':
		input_file = optarg;
		break;

	// timeout in minutes
	case 't':
		timeout_min = atoi(optarg);

		// make sure the timeout is actually positive
		if (timeout_min <= 0) {
			fprintf(stderr,
				"Invalid timeout value.\n");

			exit(EXIT_INVALID_TIMEOUT);
		}

		break;

	// number of each type of worker process to start
	case 'N':
		n_naive = atoi(optarg);
		break;

	case 'P':
		n_pollard = atoi(optarg);
		break;

	case 'F':
		n_fermat = atoi(optarg);
		break;

	case 'v':
		verbose = 1;
		break;

	case 'h':
		print_help(argv[0]);
		exit(EXIT_SUCCESS);

	default:
		fprintf(stderr,
			"Unknown option. Use -h for help.\n");

		exit(1);
	}
	}

	// cant do anything without a shared memory name
	if (shm_name == NULL) {
		fprintf(stderr,
			"Error: shared memory segment name required (-n).\n");

		exit(EXIT_NO_SHM_NAME);
	}

	total_workers = n_naive + n_pollard + n_fermat;

	// convert timeout from minutes to seconds for alarm()
	timeout_sec = timeout_min * 60;

	if (verbose) {
		fprintf(stderr,
			"n_naive=%d  n_pollard=%d  n_fermat=%d  timeout=%d sec\n",
			n_naive, n_pollard, n_fermat, timeout_sec);
	}

	// open the input file, or just use stdin
	infile = stdin;

	if (input_file != NULL) {
		infile = fopen(input_file, "r");

		if (infile == NULL) {
			perror("fopen");
			exit(1);
		}
	}

	// create the shared memory segment and set up the mutex and fields
	shm_data = create_shm(shm_name);
	init_shm(shm_data);

	// set up signal handlers
	// zeroing the struct first to avoid any garbage in sa_mask or sa_flags
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	// ctrl-c handler - sets got_sigint so we can exit cleanly
	sa.sa_handler = handle_sigint;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction SIGINT");
		exit(1);
	}

	// pyrofactor receives SIGTERM too when it sends to the process group,
	// so we need a handler that just ignores it
	sa.sa_handler = handle_sigterm;

	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sigaction SIGTERM");
		exit(1);
	}

	// SIGALRM fires when the timeout runs out
	sa.sa_handler = handle_sigalrm;

	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		perror("sigaction SIGALRM");
		exit(1);
	}

	// put pyrofactor in its own process group before forking
	// that way all the child processes inherit the same group
	// and we can send signals to all of them at once using killpg()
	// both args being 0 means: use my own pid as the new group id
	setpgid(0, 0);

	// read one number per line and try to factor each one
	while (!got_sigint &&
	       fgets(line_buf, sizeof(line_buf), infile) != NULL) {
		size_t len = strlen(line_buf);

		// remove the newline and any trailing whitespace
		while (len > 0 &&
		       (line_buf[len - 1] == '\n' || line_buf[len - 1] == '\r' ||
		        line_buf[len - 1] == ' '  || line_buf[len - 1] == '\t')) {
			line_buf[--len] = '\0';
		}

		if (len == 0) {
			continue;
		}

		// write the new value into shared memory and reset all the result fields
		// need the mutex here because workers may still be finishing up
		pthread_mutex_lock(&shm_data->mutex);

		strncpy(shm_data->current_value, line_buf, MAX_VALUE_LEN - 1);
		shm_data->current_value[MAX_VALUE_LEN - 1] = '\0';
		memset(shm_data->result_factor, 0, MAX_VALUE_LEN);
		shm_data->found              = false;
		shm_data->winning_pid        = 0;
		shm_data->winning_worker     = 0;
		shm_data->winnning_algorithm = UNKNOWN_ALGORITHM;

		pthread_mutex_unlock(&shm_data->mutex);

		// fork and exec all the worker processes
		// each one will read current_value from shared memory and start factoring
		for (i = 0; i < n_naive; i++) {
			launch_naive(shm_name, i, n_naive);
		}

		for (i = 0; i < n_pollard; i++) {
			launch_pollard(shm_name, i);
		}

		for (i = 0; i < n_fermat; i++) {
			launch_fermat(shm_name, i, n_fermat);
		}

		printf("Pyrofactor: %d Workers started.\n", total_workers);
		printf("Timeout set to %d seconds\n", timeout_sec);
		fflush(stdout);

		// reset the alarm flag and start the timer
		got_sigalrm = 0;
		alarm((unsigned int)timeout_sec);
		gettimeofday(&tv_start, NULL);

		// keep checking shared memory until someone finds a factor,
		// the alarm goes off, or we get ctrl-c
		// nanosleep wakes up early if a signal comes in, so we
		// re-check the conditions right away instead of waiting the full 10ms
		while (!got_sigint && !got_sigalrm) {
			pthread_mutex_lock(&shm_data->mutex);
			found = shm_data->found;
			pthread_mutex_unlock(&shm_data->mutex);

			if (found) {
				break;
			}

			// sleep a little so we arent just spinning the cpu
			ts.tv_sec  = 0;
			ts.tv_nsec = 10000000L;  // 10 ms
			nanosleep(&ts, NULL);
		}

		// cancel the alarm whether we found something or timed out
		alarm(0);

		elapsed = elapsed_sec(&tv_start);

		// grab the results out of shared memory before killing the workers
		pthread_mutex_lock(&shm_data->mutex);

		found      = shm_data->found;
		win_pid    = shm_data->winning_pid;
		win_worker = shm_data->winning_worker;
		win_alg    = shm_data->winnning_algorithm;

		strncpy(value,  shm_data->current_value, MAX_VALUE_LEN - 1);
		strncpy(factor, shm_data->result_factor,  MAX_VALUE_LEN - 1);
		value[MAX_VALUE_LEN - 1]  = '\0';
		factor[MAX_VALUE_LEN - 1] = '\0';

		pthread_mutex_unlock(&shm_data->mutex);

		// tell all the workers to stop
		// killpg sends to the whole process group, which includes pyrofactor
		// itself - that's ok because our SIGTERM handler is a no-op
		killpg(0, SIGTERM);

		// wait for all children to finish so we dont leave zombies
		reap_all_children();

		if (got_sigint) {
			break;
		}

		if (found) {
			printf("Factor found\n");
			printf("Results\n");
			printf("Value: %s\n", value);
			printf("Factor: %s\n", factor);
			printf("Time: %.2f sec\n", elapsed);
			printf("Winning PID: %d\n", win_pid);
			printf("Winning Worker: %d\n", win_worker);
			printf("Winning Alg: %s\n", alg_name(win_alg));
		} else {
			// nobody found a factor before the timeout
			printf("Timeout occured *****\n");
			printf("No factor identified.\n");
			printf("Value: %s\n", value);
			printf("Time: %.2f sec\n", elapsed);
		}

		printf("\n");
		fflush(stdout);
	}

	if (infile != stdin) {
		fclose(infile);
	}

	cleanup_shm();

	// spec says only unlink the shared memory on a normal exit,
	// not if we were interrupted by ctrl-c
	if (!got_sigint) {
		remove_shm();
	}

	return EXIT_SUCCESS;
}


// ctrl-c handler - sets the flag and forwards SIGINT to all workers
// spec says on SIGINT pyrofactor should send SIGINT to all worker processes
// guard against re-entrancy since killpg delivers to the whole group
// including pyrofactor itself
static void
handle_sigint(int sig)
{
	(void)sig;

	if (got_sigint) {
		return;
	}

	got_sigint = 1;
	killpg(0, SIGINT);
}


// pyrofactor sends SIGTERM to the process group when a factor is found
// so it receives the signal itself too - just ignore it
static void
handle_sigterm(int sig)
{
	(void)sig;
}


// called when the timeout alarm fires
static void
handle_sigalrm(int sig)
{
	(void)sig;

	got_sigalrm = 1;
}


// print usage info and exit
static void
print_help(const char *prog)
{
	printf("Usage: %s [options]\n", prog);

	printf("  -n <name>   Name of shared memory segment (required)\n");

	printf("  -i <file>   Input file (default: stdin)\n");

	printf("  -t <min>    Timeout in minutes (default: 2)\n");

	printf("  -N <#>      Number of naive worker processes (default: 0)\n");

	printf("  -P <#>      Number of Pollard Rho worker processes (default: 0)\n");

	printf("  -F <#>      Number of Fermat worker processes (default: 0)\n");

	printf("  -v          Verbose output to stderr\n");

	printf("  -h          Print this help and exit\n");

	exit(EXIT_SUCCESS);
}


// returns how many seconds have passed since start
static double
elapsed_sec(struct timeval *start)
{
	struct timeval now;

	gettimeofday(&now, NULL);

	return (double)(now.tv_sec  - start->tv_sec) +
	       (double)(now.tv_usec - start->tv_usec) / 1e6;
}


// converts the algorithm enum to a readable string for output
static const char *
alg_name(factor_algorithms a)
{
	switch (a) {
	case NAIVE_FACTORIZATION:
		return "Naive algorithm";

	case POLLARD_RHO_FACTORIZATION:
		return "Pollard Rho algorithm";

	case FERMAT_FACTORIZATION:
		return "de Fermat algorithm";

	default:
		return "Unknown algorithm";
	}
}


// creates the shared memory segment and maps it into our address space
static factorize_data_t *
create_shm(const char *name)
{
	factorize_data_t *ptr;

	// O_CREAT creates it if it doesnt exist, O_RDWR so we can read and write
	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0660);

	if (shm_fd < 0) {
		perror("shm_open");
		exit(EXIT_SHM_OPEN_FAIL);
	}

	// set the size of the shared memory object
	if (ftruncate(shm_fd, sizeof(factorize_data_t)) < 0) {
		perror("ftruncate");
		exit(EXIT_FRUNC_FAIL);
	}

	// map it into our address space so we can use it like a regular pointer
	ptr = mmap(NULL, sizeof(factorize_data_t),
		   PROT_READ | PROT_WRITE, MAP_SHARED,
		   shm_fd, 0);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_MMAP_FAIL);
	}

	return ptr;
}


// sets up the mutex and zeroes all the fields in the shared segment
static void
init_shm(factorize_data_t *data)
{
	pthread_mutexattr_t attr;

	// mutex needs PTHREAD_PROCESS_SHARED so worker processes can use it too
	// PTHREAD_MUTEX_ROBUST means we can recover if a process dies holding it
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
	pthread_mutex_init(&data->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	// zero out all the data fields
	memset(data->current_value, 0, MAX_VALUE_LEN);
	memset(data->result_factor, 0, MAX_VALUE_LEN);

	data->found              = false;
	data->winning_pid        = 0;
	data->winning_worker     = 0;
	data->winnning_algorithm = UNKNOWN_ALGORITHM;
}


// unmap and close the shared memory fd
static void
cleanup_shm(void)
{
	if (shm_data != NULL) {
		munmap(shm_data, sizeof(factorize_data_t));
		shm_data = NULL;
	}

	if (shm_fd >= 0) {
		close(shm_fd);
		shm_fd = -1;
	}
}


// remove the shared memory object from the system
static void
remove_shm(void)
{
	if (shm_name != NULL) {
		shm_unlink(shm_name);
	}
}


// fork and exec one naive worker
// stride is n_naive * 2 so each worker covers a different range of divisors
static void
launch_naive(const char *seg_name, int worker_id, int n_naive)
{
	pid_t pid;
	int stride;
	char w_str[32];
	char s_str[32];

	// build the argv array for execvp
	// the strings need to stay alive until after execvp, which is fine
	// since they are local and exec replaces the process image anyway
	char *args[] = {
		(char *)NAIVE_APP,
		"-n", (char *)seg_name,
		"-w", w_str,
		"-s", s_str,
		NULL
	};

	stride = n_naive * 2;

	// convert the ints to strings since exec takes string arrays
	snprintf(w_str, sizeof(w_str), "%d", worker_id);
	snprintf(s_str, sizeof(s_str), "%d", stride);

	pid = fork();

	if (pid < 0) {
		perror("fork (naive)");
		exit(1);
	}

	if (pid == 0) {
		// we are the child - replace ourselves with the naive program
		execvp(NAIVE_APP, args);

		// only gets here if exec failed
		perror("execvp naive");
		_exit(1);
	}

	// we are the parent - just log and return to keep launching workers
	if (verbose) {
		fprintf(stderr,
			"Launched naive worker %d  pid=%d  stride=%d\n",
			worker_id, (int)pid, stride);
	}
}


// fork and exec one pollard worker
// each instance gets a different worker_id so they seed differently
static void
launch_pollard(const char *seg_name, int worker_id)
{
	pid_t pid;
	char w_str[32];
	char *args[] = {
		(char *)POLLARD_APP,
		"-n", (char *)seg_name,
		"-w", w_str,
		NULL
	};

	snprintf(w_str, sizeof(w_str), "%d", worker_id);

	pid = fork();

	if (pid < 0) {
		perror("fork (pollard)");
		exit(1);
	}

	if (pid == 0) {
		execvp(POLLARD_APP, args);
		perror("execvp pollard");
		_exit(1);
	}

	if (verbose) {
		fprintf(stderr,
			"Launched pollard worker %d  pid=%d\n",
			worker_id, (int)pid);
	}
}


// fork and exec one fermat worker
// total number of fermat processes is passed as -t so each one knows
// how to divide up the work
static void
launch_fermat(const char *seg_name, int worker_id, int n_fermat)
{
	pid_t pid;
	char w_str[32];
	char t_str[32];
	char *args[] = {
		(char *)FERMAT_APP,
		"-n", (char *)seg_name,
		"-w", w_str,
		"-t", t_str,
		NULL
	};

	snprintf(w_str, sizeof(w_str), "%d", worker_id);
	snprintf(t_str, sizeof(t_str), "%d", n_fermat);

	pid = fork();

	if (pid < 0) {
		perror("fork (fermat)");
		exit(1);
	}

	if (pid == 0) {
		execvp(FERMAT_APP, args);
		perror("execvp fermat");
		_exit(1);
	}

	if (verbose) {
		fprintf(stderr,
			"Launched fermat worker %d  pid=%d  total=%d\n",
			worker_id, (int)pid, n_fermat);
	}
}


// wait for all child processes to finish
// calling this after sending SIGTERM prevents zombie processes
static void
reap_all_children(void)
{
	int status;
	pid_t p;

	// keep calling waitpid until there are no more children
	while ((p = waitpid(-1, &status, 0)) > 0) {
		if (verbose) {
			fprintf(stderr,
				"Reaped child pid=%d  status=%d\n",
				(int)p, status);
		}
	}
}
