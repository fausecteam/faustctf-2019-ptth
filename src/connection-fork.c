//#include "connection.h"
#include "request.h"
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int initConnectionHandler(void)
{
	struct sigaction action;
	action.sa_flags = SA_NOCLDWAIT | SA_RESTART;
	action.sa_handler = SIG_DFL;
	if(sigemptyset(&action.sa_mask)!= 0)
	{
		perror("sigemptyset");
	}
	if (sigaction(SIGCHLD, &action, NULL) != 0)
	{
		/*in Linux Man Pages steht nichts von einer errno,
		 * erwaehnt aber die Werte dieser.Aber in den Posix-Man Page, daher gehe ich mal davon aus
		 */
		perror("sigaction");
		fprintf(stderr, "SIGCHLD failed to set to ignore. The Server could continue, but now there are generated Zombis.");
	}

	return initRequestHandler();
}

void handleConnection(int clientSock, int listenSock)
{

	FILE *rx = fdopen(clientSock, "r");
	int sock2 = dup(clientSock);
	FILE *tx = fdopen(sock2, "w");

	pid_t p = fork();

	switch (p)
	{
	case -1:
		perror("fork");
		//exit(EXIT_FAILURE);
		fprintf(stderr,
				"fork failure only in one connection. This connection is closed, but server process tries to proceed.");
		break;

	case 0:
	{
		close(listenSock);

		handleRequest(rx, tx);

		break;
	}

	default:
		break;
		//return;

	}

	if (fclose(tx) == EOF || fclose(rx) == EOF)
	{
		perror("fclose");
		close(clientSock);
		close(sock2);
	}

	if(p==0) exit(EXIT_SUCCESS); 
	
	return;
}

