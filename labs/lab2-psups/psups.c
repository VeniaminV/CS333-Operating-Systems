// Veniamin Velikoretskikh
// CS 333 Lab 2 - psups.c

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <signal.h>

#include "psups.h"

int is_proc(const char *name);
void strip_parens(char *name);
void print_header(int extended);
int signame_to_num(char *);
int read_stat_file(char *, proc_stat_t *);

// REQUIRED GLOBAL it is used by psups_stat.c
int is_verbose = 0;



//Check if /proc entry is a PID directory
//Only digits are valid
int is_proc(const char *name)
{
    int i;

    if (name == NULL || *name == '\0')
        return 0;

    i = 0;
    while (name[i] != '\0') {
        if (!isdigit((unsigned char)name[i]))
            return 0;
        i++;
    }

    return 1;
}


//Remove parentheses around exec name
void strip_parens(char *name)
{
    size_t len;
    len = strlen(name);

    if (len > 2 && name[0] == '(' && name[len - 1] == ')') {
        memmove(name, name + 1, len - 2);
        name[len - 2] = '\0';
    }
}



//Print table header
void print_header(int extended)
{
    if (!extended) {
        printf("%-9s %-9s %-15s %-17s %-2s %-4s %-4s %-3s %10s\n",
               "pid", "ppid", "logname", "prog", "st",
               "prio", "nice", "#th", "vsize");
    } else {
        printf("%-9s %-9s %-9s %-9s %-15s %-17s %-2s %-4s %-4s %-3s %10s %s\n",
               "pid", "ppid", "pgrp", "sess", "logname", "prog",
               "st", "prio", "nice", "#th", "vsize", "command");
    }
}




int main(int argc, char *argv[])
{
    DIR *dir;
    struct dirent *entry;

    //filters / options
    uid_t filter_uid;
    char *name_filter;
    int zombie_only;
    int extended;
    int signal_to_send;

    int opt;

    struct passwd *pw;
    struct stat sb;

    // buffers for paths
    char proc_path[PATH_MAX];
    char stat_path[PATH_MAX];
    char cmd_path[PATH_MAX];

    proc_stat_t ps;
    double vsize_mb;

    FILE *cmd;
    char command[201];

    pid_t pid;


    //initialize filters
    filter_uid = UINT_MAX;
    name_filter = NULL;
    zombie_only = 0;
    extended = 0;
    signal_to_send = NO_SIGNAL;


    // comand line options 
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {

        case 'u': //filter by uid num
            filter_uid = (uid_t)atoi(optarg);
            break;

        case 'U': //filter by username
            pw = getpwnam(optarg);
            if (pw != NULL)
                filter_uid = pw->pw_uid;
            break;

        case 'm':  //current user
            filter_uid = getuid();
            break;

        case 'n':  //filter by process name
            name_filter = optarg;
            break;

        case 'z':  //only zombies
            zombie_only = 1;
            break;

        case 'x':  //entended output
            extended = 1;
            break;

        case 's':  //signal number
            signal_to_send = atoi(optarg);
            break;

        case 'S':   //signal name but converts it to number
            signal_to_send = signame_to_num(optarg);
            break;

        case 'i':  // shortcut for SIGINT
            signal_to_send = SIGINT;
            break;

        case 'K':  //shortcut for SIGKILL
            signal_to_send = SIGKILL;
            break;

        case 'v':  //verbose
            is_verbose++;
            break;
	case 'h':
    	    printf("Usage: psups [options]\n");
            printf("Options:\n");
       	    printf("  -u uid     Show only processes owned by uid\n");
    	    printf("  -U user    Show only processes owned by username\n");
    	    printf("  -m         Show only processes owned by current user\n");
    	    printf("  -n str     Show only processes with name matching str\n");
    	    printf("  -z         Show only zombie processes\n");
    	    printf("  -s signum  Send signal number to matching processes\n");
    	    printf("  -S signal  Send signal name to matching processes\n");
    	    printf("  -i         Send SIGINT to matching processes\n");
    	    printf("  -K         Send SIGKILL to matching processes\n");
    	    printf("  -x         Show extended output\n");
    	    printf("  -v         Verbose mode\n");
    	    printf("  -h         Show this help message\n");
    	    return EXIT_SUCCESS;
        }
    }

    

    //open /proc
    dir = opendir(PROC);
    if (dir == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    print_header(extended);


    // looping through proc
    while ((entry = readdir(dir)) != NULL) {

	//skip anything that isn't a PID directory
        if (!is_proc(entry->d_name))
            continue;

        //Build /proc/[pid]
        if (snprintf(proc_path, sizeof(proc_path), "%s/%s",
                     PROC, entry->d_name) >= (int)sizeof(proc_path)) {
            continue;
        }

         //stat() process directory
        if (stat(proc_path, &sb) == -1)
            continue;

        //filter by UID
        if (filter_uid != UINT_MAX && sb.st_uid != filter_uid)
            continue;

        pw = getpwuid(sb.st_uid);

        //build stat path
        if (snprintf(stat_path, sizeof(stat_path), "%s/stat",
                     proc_path) >= (int)sizeof(stat_path)) {
            continue;
        }

	// read process info from stat file
        if (read_stat_file(stat_path, &ps) == 0)
            continue;

        strip_parens(ps.exec_name);

        //filter by process name
        if (name_filter != NULL && strcmp(name_filter, ps.exec_name) != 0)
            continue;

        //filter zombies
        if (zombie_only && ps.state != ZOMBIE_CHAR)
            continue;

         //SEND SIGNAL MODE
        if (signal_to_send != NO_SIGNAL) {
            pid = (pid_t)strtoul(entry->d_name, NULL, 10);
            kill(pid, signal_to_send);
            continue;
        }

        //convert memory to MB
        vsize_mb = ps.vsize / (1024.0 * 1024.0);

	//output
        if (!extended) {
            printf("%-9d %-9d %-15s %-17s %-2c %-4ld %-4ld %-3ld %10.2f\n",
                   ps.pid,
                   ps.ppid,
                   pw ? pw->pw_name : "unknown",
                   ps.exec_name,
                   ps.state ? ps.state : ' ',
                   ps.priority,
                   ps.nice,
                   ps.num_threads,
                   vsize_mb);
        }


	//extended output (-x)
        else {

            command[0] = '\0';

            if (snprintf(cmd_path, sizeof(cmd_path), "%s/cmdline",
                         proc_path) < (int)sizeof(cmd_path)) {

                cmd = fopen(cmd_path, "r");

                if (cmd != NULL) {

                    size_t len;

                    len = fread(command, 1, 200, cmd);
                    fclose(cmd);

                    //replace NULLs with spaces
                    for (size_t i = 0; i < len; i++) {
                        if (command[i] == '\0')
                            command[i] = ' ';
                    }

                    command[200] = '\0';
                }
            }

            printf("%-9d %-9d %-9d %-9d %-15s %-17s %-2c %-4ld %-4ld %-3ld %10.2f %s\n",
                   ps.pid,
                   ps.ppid,
                   ps.pgrp,
                   ps.session,
                   pw ? pw->pw_name : "unknown",
                   ps.exec_name,
                   ps.state ? ps.state : ' ',
                   ps.priority,
                   ps.nice,
                   ps.num_threads,
                   vsize_mb,
                   command);
        }
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
