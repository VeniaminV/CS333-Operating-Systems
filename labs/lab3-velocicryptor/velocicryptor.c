// Veniamin Velikoretskikh veniamin@pdx.edu
// CS333 Lab3 - velocicryptor
// Multi-threaded password cracker using crypt_rn()

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <crypt.h>
#include "velocicryptor.h"

#define OPTIONS "i:d:o:t:n:vh"
#define NS_PER_SECOND 1000000000.0


// Number of threads user requested
static int num_threads = 1;
static int verbose = 0;

// List of hashed passwords
static char **hashes = NULL;
static int num_hashes = 0;

// List of plaintext passwords
static char **passwords = NULL;
static int num_passwords = 0;

// Per-process totals (protected by stats_lock)
static int total_counts[NUM_TYPES] = {0};
static int total_failed = 0;
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

// Mutex protecting output so lines don't interleave across threads
static pthread_mutex_t out_lock = PTHREAD_MUTEX_INITIALIZER;

// Next hash index to process (protected by hash_lock inside get_next_hash)
static FILE *out_fp = NULL;

// Forward declarations
static void *crack_passwords(void *arg);
static double elapse_time(struct timespec *t0, struct timespec *t1);
static int get_next_hash(void);
static char **read_file(const char *filename, int *count);
static int get_hash_type(const char *hash);
static void print_usage(const char *prog);


int
main(int argc, char *argv[])
{
    pthread_t *workers = NULL;

    char *hash_file = NULL;
    char *dict_file = NULL;
    char *out_file  = NULL;

    struct timespec et0;
    struct timespec et1;

    int opt;
    long tid;

    out_fp = stdout;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 'i':
            hash_file = optarg;
            break;
        case 'd':
            dict_file = optarg;
            break;
        case 'o':
            out_file = optarg;
            break;
        case 't':
            sscanf(optarg, "%d", &num_threads);
            break;
        case 'n':
            nice(atoi(optarg));
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
    }

    // Validate required options
    if (!hash_file || !dict_file) {
        fprintf(stderr, "Error: -i hashfile and -d dictfile are required\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (num_threads < 1) {
        fprintf(stderr, "Error: number of threads must be >= 1\n");
        exit(EXIT_FAILURE);
    }

    // Open output file if specified
    if (out_file) {
        out_fp = fopen(out_file, "w");
        if (!out_fp) {
            perror("fopen output");
            exit(EXIT_FAILURE);
        }
    }

    if (verbose) {
        fprintf(stderr, "Loading hash file: %s\n", hash_file);
        fprintf(stderr, "Loading dictionary file: %s\n", dict_file);
        fprintf(stderr, "Using %d thread(s)\n", num_threads);
    }

    // Read input files
    hashes    = read_file(hash_file, &num_hashes);
    passwords = read_file(dict_file, &num_passwords);

    if (verbose) {
        fprintf(stderr, "Loaded %d hashes and %d passwords\n",
                num_hashes, num_passwords);
    }

    // Allocate thread handles
    workers = malloc((size_t)num_threads * sizeof(pthread_t));
    if (!workers) {
        perror("malloc workers");
        exit(EXIT_FAILURE);
    }

    // Start timing
    clock_gettime(CLOCK_MONOTONIC, &et0);

    // Launch threads
    for (tid = 0; tid < num_threads; ++tid) {
        int ret = pthread_create(&workers[tid], NULL,
                                 crack_passwords, (void *)tid);
        if (ret != 0) {
            perror("pthread_create");
            fprintf(stderr, "thread creation failed for tid %ld\n", tid);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(workers[i], NULL);
    }

    // Stop timing
    clock_gettime(CLOCK_MONOTONIC, &et1);

    // Print process-level summary to stderr (order matches spec)
    fprintf(stderr, "process time %d %.2lf sec\n",
            num_threads, elapse_time(&et0, &et1));
    fprintf(stderr, "process DES %d\n",           total_counts[TYPE_DES]);
    fprintf(stderr, "process NT %d\n",            total_counts[TYPE_NT]);
    fprintf(stderr, "process MD5 %d\n",           total_counts[TYPE_MD5]);
    fprintf(stderr, "process SHA256 %d\n",        total_counts[TYPE_SHA256]);
    fprintf(stderr, "process SHA512 %d\n",        total_counts[TYPE_SHA512]);
    fprintf(stderr, "process YESCRYPT %d\n",      total_counts[TYPE_YESCRYPT]);
    fprintf(stderr, "process GOST_YESCRYPT %d\n", total_counts[TYPE_GOST_YESCRYPT]);
    fprintf(stderr, "process BCRYPT %d\n",        total_counts[TYPE_BCRYPT]);

    {
        int total = 0;
        for (int i = 0; i < NUM_TYPES; i++) {
            total += total_counts[i];
        }
        fprintf(stderr, "process total %d\n",  total);
        fprintf(stderr, "process failed %d\n", total_failed);
    }

    // Cleanup
    pthread_mutex_destroy(&stats_lock);
    pthread_mutex_destroy(&out_lock);

    free(workers);

    for (int i = 0; i < num_hashes; ++i) {
        free(hashes[i]);
    }
    free(hashes);

    for (int i = 0; i < num_passwords; ++i) {
        free(passwords[i]);
    }
    free(passwords);

    if (out_file && out_fp) {
        fclose(out_fp);
    }

    return 0;
}


// Thread function: grabs hashes dynamically and tries every dictionary word
static void *
crack_passwords(void *arg)
{
    long tid = (long)arg;

    int processed = 0;
    int failed    = 0;
    int counts[NUM_TYPES] = {0};

    struct timespec et0;
    struct timespec et1;
    struct crypt_data cd;
    char *hash;
    int cracked ;
    int htype ;
    char *res;

    clock_gettime(CLOCK_MONOTONIC, &et0);

    while (1) {
        int idx = get_next_hash();
        if (idx >= num_hashes) {
            break;  // No more hashes to process
        }

        hash   = hashes[idx];
        cracked  = 0;
        htype    = get_hash_type(hash);

        if (verbose) {
            fprintf(stderr, "[thread %ld] attempting hash #%d: %.20s...\n",
                    tid, idx, hash);
        }

        // Always count the hash type regardless of whether we crack it
        counts[htype]++;

        // Try every dictionary password against this hash
        for (int i = 0; i < num_passwords; ++i) {
            // Zero the crypt_data struct before each call (required by crypt_rn)
            memset(&cd, 0, sizeof(cd));

            res = crypt_rn(passwords[i], hash, &cd, (int)sizeof(cd));

            if (res && strcmp(res, hash) == 0) {
                // Found a match — write to out_fp under lock so output doesn't interleave
                pthread_mutex_lock(&out_lock);
                fprintf(out_fp, "cracked %s %s\n", passwords[i], hash);
                pthread_mutex_unlock(&out_lock);

                cracked = 1;
                break;
            }
        }

        if (!cracked) {
            pthread_mutex_lock(&out_lock);
            fprintf(out_fp, "*** failed to crack %s\n", hash);
            pthread_mutex_unlock(&out_lock);
            failed++;
        }

        processed++;
    }

    clock_gettime(CLOCK_MONOTONIC, &et1);

    // Per-thread stats to stderr (order matches spec)
    fprintf(stderr, "thread time %ld %.2lf sec\n", tid, elapse_time(&et0, &et1));
    fprintf(stderr, "thread %ld DES %d\n",           tid, counts[TYPE_DES]);
    fprintf(stderr, "thread %ld NT %d\n",            tid, counts[TYPE_NT]);
    fprintf(stderr, "thread %ld MD5 %d\n",           tid, counts[TYPE_MD5]);
    fprintf(stderr, "thread %ld SHA256 %d\n",        tid, counts[TYPE_SHA256]);
    fprintf(stderr, "thread %ld SHA512 %d\n",        tid, counts[TYPE_SHA512]);
    fprintf(stderr, "thread %ld YESCRYPT %d\n",      tid, counts[TYPE_YESCRYPT]);
    fprintf(stderr, "thread %ld GOST_YESCRYPT %d\n", tid, counts[TYPE_GOST_YESCRYPT]);
    fprintf(stderr, "thread %ld BCRYPT %d\n",        tid, counts[TYPE_BCRYPT]);
    fprintf(stderr, "thread %ld total %d\n",         tid, processed);
    fprintf(stderr, "thread %ld failed %d\n",        tid, failed);

    // Accumulate into process-level totals
    pthread_mutex_lock(&stats_lock);
    for (int i = 0; i < NUM_TYPES; i++) {
        total_counts[i] += counts[i];
    }
    total_failed += failed;
    pthread_mutex_unlock(&stats_lock);

    pthread_exit(NULL);
}


// Returns the next hash index to process; thread-safe via mutex
static int
get_next_hash(void)
{
    static int next_hash = 0;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    int idx;

    pthread_mutex_lock(&lock);
    idx = next_hash++;
    pthread_mutex_unlock(&lock);

    return idx;
}


// Elapsed time in seconds between two timespecs
static double
elapse_time(struct timespec *t0, struct timespec *t1)
{
    return (double)(t1->tv_sec  - t0->tv_sec)
         + (double)(t1->tv_nsec - t0->tv_nsec) / NS_PER_SECOND;
}


// Returns the hash type index for the given hash string
static int
get_hash_type(const char *hash)
{
    if (hash[0] != '$')             return TYPE_DES;
    if (strncmp(hash, "$3$",  3) == 0) return TYPE_NT;
    if (strncmp(hash, "$1$",  3) == 0) return TYPE_MD5;
    if (strncmp(hash, "$5$",  3) == 0) return TYPE_SHA256;
    if (strncmp(hash, "$6$",  3) == 0) return TYPE_SHA512;
    if (strncmp(hash, "$y$",  3) == 0) return TYPE_YESCRYPT;
    if (strncmp(hash, "$gy$", 4) == 0) return TYPE_GOST_YESCRYPT;
    if (strncmp(hash, "$2b$", 4) == 0) return TYPE_BCRYPT;
    return TYPE_DES;  // fallback
}


// Reads a file line-by-line into a heap-allocated array of strings
static char **
read_file(const char *filename, int *count)
{
    char **list  = NULL;
    char buffer[512];
    int  size    = 0;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, (int)sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';

        // Skip empty lines
        if (buffer[0] == '\0') {
            continue;
        }

        list = realloc(list, (size_t)(size + 1) * sizeof(char *));
        if (!list) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        list[size] = strdup(buffer);
        if (!list[size]) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        size++;
    }

    fclose(fp);
    *count = size;
    return list;
}


// Print usage information
static void
print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s -i hashfile -d dictfile [-o outfile] "
                    "[-t threads] [-n nice] [-v] [-h]\n", prog);
    fprintf(stderr, "  -i filename  Input file of password hashes (required)\n");
    fprintf(stderr, "  -d filename  Dictionary file of plaintext passwords (required)\n");
    fprintf(stderr, "  -o filename  Output file (default: stdout)\n");
    fprintf(stderr, "  -t #         Number of threads (default: 1)\n");
    fprintf(stderr, "  -n #         Nice value for the process\n");
    fprintf(stderr, "  -v           Enable verbose logging to stderr\n");
    fprintf(stderr, "  -h           Print this help and exit\n");
}

