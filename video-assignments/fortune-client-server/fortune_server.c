// gcc -Wall -Werror -g -o fortune_server fortune_server.c

// ./fortune_server -p 50001

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#define LISTENQ 5
#define DEFAULT_SERVER_PORT 50001
#define LINE_LEN 100

int
main(int argc, char *argv[])
{
	int listenfd = -1;
	int port = DEFAULT_SERVER_PORT;
	socklen_t clilen = 0;
	struct sockaddr_in6 servaddr;
	struct sockaddr_in6 cliaddr;

	{
		int opt = 0;

		while ((opt = getopt(argc, argv, "p:vh")) != -1){
			switch(opt){
			case 'p':
				sscanf(optarg, "%d", &port);
				break;
			case 'h':
				fprintf(stderr, "Usage: %s [-p port_num] [-h]\n", argv[0]);
				exit(EXIT_FAILURE);
				break;
			default: /* '?' */
				fprintf(stderr, "INvalid commend line option.\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	listenfd = socket(AF_INET6, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_flowinfo = 0;
	servaddr.sin6_addr = in6addr_any;
	servaddr.sin6_port = htons(port);
	
	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0){
		perror("Something is not working");
		exit(EXIT_FAILURE);
	}
	listen(listenfd, LISTENQ);
	clilen = sizeof(cliaddr);
	for ( ; ; ) {
		FILE *fp = NULL;
		char fortune[LINE_LEN] = {'\0'};
		int sockfd = -1;

		sockfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		fp = popen("fortune", "r");
		while ( fgets(fortune, LINE_LEN, fp) != NULL) {
			write(sockfd, fortune, strlen(fortune));	
		}
		pclose(fp);
		close(sockfd);
	}


	printf("Closing fortune socket\n");
	close (listenfd);

	return EXIT_SUCCESS;
}
