//Veniamin Velikoretskikh veniamin@pdx.edu

//enctypt-w-symlink

//This is what i used to test it
// ./encrypt-w-symlink -i input.txt -o output.enc -s test-symlink -v

//will read the data from an input file and engcypt with XOR
//Will use the name of the symbolik link name as key 
//also creates encrypted data to an output file

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
        char *input_file = NULL;
        char *output_file = NULL;
        char *symlink_name = NULL;
        int verbose = 0;

        int in_fd;
        int out_fd;

        char buffer[BUFFER_SIZE];
        ssize_t bytes;

        size_t key_len;
        size_t key_index = 0;

	
	//Parse command options
	/*
	-i input file, -o output file
	-s symlink name, -v verbose
	-h help
	*/
	while ((opt = getopt(argc, argv, "i:o:s:vh")) != -1)
	{
  		if (opt == 'i')
           		input_file = optarg;
        	else if (opt == 'o')
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
	//need to check if the files actually exist
	if(output_file == NULL || symlink_name == NULL)
	{
		fprintf(stderr, "Error: -o, and -s are required\n");
		return 1;
	}	
	
	//opening the input file
    	if (input_file)
    	{
        	in_fd = open(input_file, O_RDONLY);
        	if (in_fd < 0)
        	{
            		perror("open input");
            		return 1;
        	}
    	}
    	else
    	{
        	in_fd = STDIN_FILENO;
    	}


	//opening output file
    	out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    	if (out_fd < 0)
    	{
        	perror("open output");
        	if (input_file) close(in_fd);
        	return 1;
    	}


	//seting up the key
	key_len = strlen(symlink_name);

	//keep track of key
	key_index = 0;

	if(verbose)
	{
		fprintf(stderr, "Key: %s\n", symlink_name);
        	fprintf(stderr, "Key length: %lu\n", (unsigned long)key_len);
    	}

	while ((bytes = read(in_fd, buffer, BUFFER_SIZE)) > 0)
    	{
        	for (ssize_t i = 0; i < bytes; i++)
        	{
            		// XOR current byte with current key character
            		buffer[i] ^= symlink_name[key_index];

            		key_index++;

            		if (key_index >= key_len)
            		{
                		key_index = 0;
            		}
        	}

        	if (write(out_fd, buffer, bytes) != bytes)
        	{
            		perror("write");
            		if(input_file) close(in_fd);
            		close(out_fd);
            		return 1;
        	}
    	}

    	if (bytes < 0)
    	{
        	perror("read");
    	}
	
    	if(input_file) close(in_fd);
    	close(out_fd);


    	// If symlink already exists, remove it first
    	unlink(symlink_name);

    	if (symlink(output_file, symlink_name) < 0)
    	{
        	perror("symlink");
        	return 1;
    	}

    	if (verbose)
    	{
        	fprintf(stderr, "Created symlink: %s -> %s\n",
                	symlink_name, output_file);
    	}

    	return 0;
	

}

