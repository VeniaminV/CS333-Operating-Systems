//Veniamin Velikoretskikh veniamin@pdx.edu

// decrypt-w-inode.c
// This program decrypts a file using XOR with the inode key

// ./decrypt-w-inode -i output.enc -o input.txt -v


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
	int opt;
	char *input_file = NULL;
	char *output_file = NULL;
	int verbose = 0;

	int in_fd;
	int out_fd;
	struct stat st;
	unsigned char key;
	
	char buffer[BUFFER_SIZE];
	ssize_t bytes;
	
	//parse the command line 

	while ((opt = getopt(argc, argv, "i:o:vh")) != -1)
	{
  		if (opt == 'i')
           		input_file = optarg;
        	else if (opt == 'o')
            		output_file = optarg;
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
	
	//Cheching for input file
	if (input_file == NULL)
    	{
        	fprintf(stderr, "Error: -i is required\n");
        	return 1;
    	}

	//openning the input file
	in_fd = open(input_file, O_RDONLY);
	if(in_fd <0)
	{
		perror("open input");
		return 1;
	}
	
	//getting the inode from the file 
	if(fstat(in_fd, &st) < 0)
	{
		perror("fstat");
		return 1;
	}

	
	//encryption key is the low byte of the inode
	key = st.st_ino & 0xFF;	
	//printf("Key = %d\n", key);
	if(verbose)
	{
		fprintf(stderr, "Endcryption key: %d\n", key);
	}
	
	//Open the file, if no file the stdin
	//opening the output file, write only, create if not, if already exists
	//then erase. 0664 is other read only, group read and write , owner read and write
	if(output_file)
	{
		out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if(out_fd < 0) 
		{
			perror("open output");
			return 1;
		}
	}
	else
	{
		out_fd = STDOUT_FILENO;
	}
	
	
	while((bytes = read(in_fd, buffer, BUFFER_SIZE)) > 0)
	{
		for(int i = 0; i < bytes; i++)
		{
			buffer[i] ^= key;  //This is the XOR encryption	
		}

		if(write(out_fd, buffer, bytes) != bytes)
		{
			perror("write");
			return 1;
		}
	}

	//close the files	
	close(in_fd);
	close(out_fd);

	return 0;
	
}






