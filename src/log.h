#ifndef SISTER_LOG_H
#define SISTER_LOG_H

enum log_badboy {CLIENT, SERVER};

void logmsg(enum log_badboy, char *fmt, ...);

#endif
