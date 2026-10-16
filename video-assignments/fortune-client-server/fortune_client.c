// gcc -Wall -Werror -g -o fortune_client fortune_client.c

// ./fortune_client -a 131.252.208.23 -p 50001

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define LINE_LEN 100
#define DEFAULT_SERV_PORT 50001

int 
main(int argc, char *argv[])
{
	int port = DEFAULT_SERV_PORT;
	char input[LINE_LEN] = {'\0'};
	char *ip_adr = NULL;

	{
	int opt = 0;
	
	while ((opt = getopt(argc, argv, "a:p:h")) != -1){
		switch(opt){
		case 'a':
			ip_adr = optarg;
			break;
		case 'p':
			sscanf(optarg, "%d", &port);
			break;
		case 'h':
			fprintf(stderr, "Usage %s -a address -p port_num [-h]\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		default:
			fprintf(stderr, "Invalid commend line option. \n");
			exit(EXIT_FAILURE);
			break;

		}
	}
	}

	if(ip_adr == NULL) {
		fprintf(stderr, "Must provide ip address of fortune server (IPv4 or IPv6.\n");
		exit(EXIT_FAILURE);
	}
	if(port < 1024){
		fprintf(stderr, "Must provide listening port of fortune server.\n");
		exit(EXIT_FAILURE);
	}

	do{

		int sockfd;
		char fortune[LINE_LEN];
		ssize_t bytes_read = 0;
		struct sockaddr_in servaddr;
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(port);
		inet_pton(AF_INET, ip_adr, &servaddr.sin_addr.s_addr);

		if( connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))!= 0){
			perror("The fortune teller is o a break.");
			exit(EXIT_FAILURE);
		}
		while ((bytes_read = read(sockfd, fortune, LINE_LEN ))>0){
			write(STDOUT_FILENO, fortune, bytes_read);
		}
		close(sockfd);
	

	} while ((fgets(input, sizeof(input), stdin) != NULL) && (strcasecmp(input, "exit\n") != 0));




	return EXIT_SUCCESS;
}





