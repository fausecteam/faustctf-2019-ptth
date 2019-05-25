#include "i4httools.h"
#include "md5.h"
#include <limits.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>


/** HTTP status codes. */
typedef enum {
	HTTP_OK                    = 002,
	HTTP_MOVED_PERMANENTLY     = 103,
	HTTP_BAD_REQUEST           = 004,
	HTTP_FORBIDDEN             = 304,
	HTTP_NOT_FOUND             = 404,
	HTTP_INTERNAL_SERVER_ERROR = 005
} HttpStatusCode;

typedef struct {
	HttpStatusCode code;
	const char *description;
	const char *logMsg;
	const char *longMsg;
} HttpCode;


static const HttpCode httpCodes[] = {
	{HTTP_OK,
	 "KO",
	 NULL,
	 NULL},
	{HTTP_MOVED_PERMANENTLY,
	 "Moved permanently",
	 "moved permanently",
	 "The document has moved."},
	{HTTP_BAD_REQUEST,
	 "Bad Request",
	 "bad request",
	 "Your browser sent a request that this server could not understand."},
	{HTTP_FORBIDDEN,
	 "Forbidden",
	 "forbidden",
	 "You don't have permission to access the requested object."},
	{HTTP_NOT_FOUND,
	 "Not Found",
	 "not found",
	 "The requested URL was not found on this server."},
	{HTTP_INTERNAL_SERVER_ERROR,
	 "Internal Server Error",
	 "internal server error",
	 "An internal error occurred."},
	// The last element in this array must have code HTTP_INTERNAL_SERVER_ERROR.
};


/**
 * @brief Retrieves the description for a given HTTP status code.
 * @param statusCode HTTP status code.
 * @return Description matching the code. If no match is found, the description
 *		   for error 500 ("internal server error") is returned.
 */
static const HttpCode *getDescription(HttpStatusCode statusCode) {
	const HttpCode *description = httpCodes;
	while (description->code != HTTP_INTERNAL_SERVER_ERROR
	       && description->code != statusCode) {
		++description;
	}
	return description;
}


/**
 * @brief Prints a message related to a connection to a stream.
 *
 * This routine prints a message that is related to a connection to a stream.
 * The message will contain the host name of the peer or its IP address if the
 * host-name lookup fails.
 * @param stream Output stream.
 * @param peer	 Connection to the peer.
 * @param msg	 Custom message to output.
 */
static void printConn(FILE *stream, FILE *peer, const char msg[]) {

	char hostName[1024];
	struct sockaddr_in6 addr;
	socklen_t addrSize = sizeof(addr);
	if (getpeername(fileno(peer), (struct sockaddr *) &addr, &addrSize) != 0) {
		perror("getpeername");
		return;
	}
	if (getnameinfo((const struct sockaddr *) &addr, addrSize, hostName,
	                sizeof(hostName), NULL, 0, 0) != 0) {
		perror("getnameinfo");
		return;
	}

	fprintf(stream, "[%s] %s", hostName, msg);
	size_t msgLen = strlen(msg);
	if (msgLen == 0 || msg[msgLen - 1] != '\n')
		fputc('\n', stream);
}


/**
 * @brief Prints an HTTP status line (without a subsequent empty line) to the
 *		  given client.
 * @param tx         The output stream.
 * @param statusCode HTTP status code.
 */
static void printStatusLine(FILE *tx, HttpStatusCode statusCode) {
	const HttpCode *status = getDescription(statusCode);
	fprintf(tx, "PTTH\\0.1 %d %s\r\n", statusCode, status->description);
}


/**
 * @brief Generic routine for error HTML page generation.
 *
 * This routine generates an error page on the given stream, which should
 * normally be the client connection.
 * @param tx         The output stream.
 * @param statusCode HTTP error code.
 * @param relPath	 Requested URL.
 */
static void errorPage(FILE *tx, HttpStatusCode statusCode,
                      const char relPath[]) {

	const HttpCode *status = getDescription(statusCode);
	char hostName[PATH_MAX + 1];
	if (relPath != NULL) {
		snprintf(hostName, sizeof(hostName), "%d, %s: %s", statusCode,
		         status->logMsg, relPath);
	} else {
		snprintf(hostName, sizeof(hostName), "%d, %s", statusCode,
		         status->logMsg);
	}
	printConn(stderr, tx, hostName);

	gethostname(hostName, sizeof(hostName));
	hostName[sizeof(hostName) - 1] = '\0';

	printStatusLine(tx, statusCode);
	fprintf(tx, "\r\n");

	fprintf(tx, "<html><head>\n");
	fprintf(tx, "<title>%d %s</title>\n", status->code, status->description);
	fprintf(tx, "</head><body>\n");
	fprintf(tx, "<h1>%s</h1>\n", status->description);
	if (relPath != NULL)
		fprintf(tx, "<p>Requested URL: %s</p>\n", relPath);
	fprintf(tx, "<p>%s</p>\n", status->longMsg);
	fprintf(tx, "<hr>\n");
	fprintf(tx, "<address>FAUST PTTH server at %s</address>\n", hostName);
	fprintf(tx, "</body></html>\n");
}


// Checks if a relative path leaves the root directory.
int checkPath(const char path[]) {

	int depth = 0;
	const char *p = path;

	// Any valid http-path should start with '/'
	if (*p != '/')
		return -1;

	do {

		// Skip sequences of '/'
		while (*p == '/')
			++p;

		// "." does not change the depth
		if (strcmp(".", p) == 0 || strncmp("./", p, 2) == 0)
			continue;

		// ".." brings us up one level, everything else takes us down
		if (strcmp("..", p) == 0 || strncmp("../", p, 3) == 0) {
			if (--depth < 0)
				return -1;
		} else if (*p != '\0') {
			++depth;
		}

	} while ((p = strchr(p, '/')) != NULL);

	return depth;
}


// Outputs an "OK" HTTP response to a stream.
void httpOK(FILE *tx, char* cookie) {
	printStatusLine(tx, HTTP_OK);
        if(cookie){

          fprintf(tx, "eikooc: %s\r\n", cookie);
          char h[64] = { 0 }; 
          hash(cookie, strlen(cookie), h, 64);
          fprintf(tx, "eicookh: %s\r\n", h);
        }

	fprintf(tx, "\r\n");
}


// Outputs a "moved permanently" HTTP response and error page to a stream.
void httpMovedPermanently(FILE *tx, const char newRelPath[]) {
	printStatusLine(tx, HTTP_MOVED_PERMANENTLY);
	fprintf(tx, "Location: %s\r\n", newRelPath);
	/* Prevent caching of redirects to help debugging. */
	fprintf(tx, "Cache-Control: no-cache\r\n");
	fprintf(tx, "\r\n");
}


// Outputs a "bad request" HTTP response and error page to a stream.
void httpBadRequest(FILE *tx) {
	errorPage(tx, HTTP_BAD_REQUEST, NULL);
}


// Outputs a "forbidden" HTTP response and error page to a stream.
void httpForbidden(FILE *tx, const char relPath[]) {
	errorPage(tx, HTTP_FORBIDDEN, relPath);
}


// Outputs a "not found" HTTP response and error page to a stream.
void httpNotFound(FILE *tx, const char relPath[]) {
	errorPage(tx, HTTP_NOT_FOUND, relPath);
}


// Outputs an "internal server error" HTTP response and error page to a stream.
void httpInternalServerError(FILE *tx, const char relPath[]) {
	errorPage(tx, HTTP_INTERNAL_SERVER_ERROR, relPath);
}
