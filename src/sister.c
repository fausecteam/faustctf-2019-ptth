// big thx to https://www4.cs.fau.de/ (especially chris)!
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "cmdline.h"
#include "connection.h"
#include "i4httools.h"
#include "request.h"

static void printUsage(char* argv0, char* opterror)
{
	if (opterror == NULL)
	{
		opterror = "";
	}

	fprintf(stderr, "%s Usage: %s --wwwpath=<dir> [--port=<p>] \n", opterror,
			argv0);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{

	if (argc < 2 || argc > 3 || cmdlineInit(argc, argv) != 0)
	{
		printUsage(argv[0], NULL);
	}

	/*	char* wwwpath = cmdlineGetValueForKey("wwwpath");
	 if( wwwpath == NULL)
	 {
	 fprintf(stderr, "Wrong Arguments. ");
	 printUsage(argv[0]);
	 }
	 DIR* testdir = opendir(wwwpath);

	 if(testdir== NULL)
	 {q
	 fprintf(stderr, "'wwwpath' is not valid!");
	 exit(EXIT_FAILURE);
	 }
	 if(closedir(testdir)!=0)
	 {
	 fprintf(stderr, "Error while testing wwwpath.\n");
	 perror("closedir");
	 exit(EXIT_FAILURE);
	 }
	 */
	uint16_t port = 2013;
	const char* portArg = cmdlineGetValueForKey("port");
	if(portArg!=NULL)
	{
		long porttmp = strtol(portArg, NULL, 10);
		if (porttmp <=0 || porttmp > 0xffff)
		{
			fprintf(stderr, "portnumber is not valid");
			exit(EXIT_FAILURE);
		}
		else
		{
			port = (uint16_t) porttmp;
		}

		
	}
	
	
	if (initConnectionHandler() != 0)
	{
		printUsage(argv[0], "Invalid Parameters!");
	}

	int sock = socket(AF_INET6, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	/*struct in6_addr IP6_addresse = {
	 unsigned char s6_addr[16],
	 };*/
	int flag = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));


	struct sockaddr_in6 sockAddresse = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port),
			.sin6_flowinfo = 0,
			.sin6_addr = in6addr_any,
			.sin6_scope_id = 0,
	};


	if(bind(sock, (struct sockaddr*) &sockAddresse, sizeof(sockAddresse))!=0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	printf("bind successful");
#endif

	if(listen(sock, SOMAXCONN)!=0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	int newSocket;
	while((newSocket = accept(sock, NULL, NULL ))!= -1)
	{
		handleConnection(newSocket, sock); //wirklich eigener sock?
	}

	perror("accept");
	fprintf(stderr, "Server will terminate immediately; All Connections were closed.");
	exit(EXIT_FAILURE);

}

typedef void (*signal_handler_t)(int);


