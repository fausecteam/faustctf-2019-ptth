#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

#define LOGMAX (256)

void logmsg(enum log_badboy bb, char *fmt, ...)
{
	va_list ap;
	
	char msg[LOGMAX];

	va_start(ap, fmt);
	size_t s = vsnprintf(msg, LOGMAX, fmt, ap);
	va_end(ap);
	fprintf(stderr, "[PID %d] [%s] %s %s\n", (int)getpid(),
			(s==LOGMAX?"...":""), msg, (bb==SERVER?" SERVER":"CLIENT"));
	fprintf(stderr, "\n");
}

