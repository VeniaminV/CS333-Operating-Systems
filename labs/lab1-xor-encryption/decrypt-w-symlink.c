//Veniamin Velikoretskikh veniamin@pdx.edu

//decrupt-w-symlink

//how i test it
// ./decrypt-w-symlink -s test-symlink -o decrypted.txt -v

//dectypts a file using XOR encryption, the key is the name of the sym link

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>    
#include <sys/stat.h>  
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int
main(int argc, char *argv[])
{
        int opt;
//        char *input_file = NULL;
        char *output_file = NULL;
        char *symlink_name = NULL;
        int verbose = 0;

        int in_fd;
        int out_fd;

        char buffer[BUFFER_SIZE];
        ssize_t bytes;

        size_t key_len;
        size_t key_index = 0;

	char target_file[BUFFER_SIZE];
	ssize_t len;


	
	//Parse command options
	/*
	-o output file
	-s symlink name, -v verbose
	-h help
	*/
	while ((opt = getopt(argc, argv, "o:s:vh")) != -1)
	{
        	if (opt == 'o')
            		output_file = optarg;
		else if(opt == 's')
			symlink_name = optarg;
        	else if (opt == 'v')
            		verbose = 1;
        	else if (opt == 'h')
        	{
            		printf("Usage: %s -i input -o output [-v]\n", argv[0]);
            		return 0;
        	}
        	else
        	{
            		fprintf(stderr, "Invalid option\n");
            		return 1;
        	}
	}

	//check for sim link if it exists
	if (symlink_name == NULL)
	{
		fprintf(stderr, "Error: -s is required\n");
		return 1;
	}

	//getting the file from the sym link
	len = readlink(symlink_name, target_file, sizeof(target_file) - 1);
    	if (len < 0)
    	{
        	perror("readlink");
        	return 1;
    	}

    	target_file[len] = '\0';

    	if (verbose)
    	{
        	fprintf(stderr, "Symlink: %s\n", symlink_name);
        	fprintf(stderr, "Points to: %s\n", target_file);
    	}

	//opening the encrypted input file
	in_fd = open(target_file, O_RDONLY);
    	if (in_fd < 0)
    	{
        	perror("open input");
        	return 1;
    	}

	//writing to output
	if (output_file)
    	{
        // Permissions, rw-rw-r-- (0664)
        	out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0664);
        	if (out_fd < 0)
        	{
            		perror("open output");
            		close(in_fd);
            		return 1;
        	}
    	}
    	else
    	{
        	out_fd = STDOUT_FILENO;
    	}



    	// Key is the name of the symlink
    	key_len = strlen(symlink_name);
    	key_index = 0;

    	if (verbose)
    	{
        	fprintf(stderr, "Key: %s\n", symlink_name);
        	fprintf(stderr, "Key length: %lu\n", (unsigned long)key_len);
    	}

    	// decrypting
    	while ((bytes = read(in_fd, buffer, BUFFER_SIZE)) > 0)
    	{
        	// XOR each byte with key
        	for (ssize_t i = 0; i < bytes; i++)
        	{
            		buffer[i] ^= symlink_name[key_index];
	
            		key_index++;
            		if (key_index >= key_len)
                	key_index = 0;
        	}

        	if (write(out_fd, buffer, bytes) != bytes)
        	{
            		perror("write");
            		close(in_fd);
            		if (output_file) close(out_fd);
            		return 1;
        	}
    	}
	
    	if (bytes < 0)
    	{
        	perror("read");
    	}

    	close(in_fd);
    	if (output_file) close(out_fd);

    	return 0;
}
