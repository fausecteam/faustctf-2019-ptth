#include "cmdline.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


typedef enum {
	KEY_VALUE_PAIR,
	FLAG,
	EXTRA_ARG
} ArgType;


static unsigned int argCount = 0;
static char		  **args	 = NULL;


static ArgType classify(const char s[]) {
	const char *p = s;
	if (*p != '-')
		return EXTRA_ARG;
	if (*++p == '-')
		++p;
	if (isalnum(*p) == 0)
		return EXTRA_ARG;
	if (strchr(p, '=') == NULL)
		return FLAG;
	return KEY_VALUE_PAIR;
}


static const char *skipLeadingDashes(const char s[]) {
	if (*s == '-') {
		if (*++s == '-')
			++s;
	}
	return s;
}


// Initializes the command-line parsing module.
int cmdlineInit(int argc, char *argv[]) {
	if (argc < 1 || argv == NULL) {
		errno = EINVAL;
		return -1;
	}
	argCount = (unsigned int) argc;
	args	 = argv;
	return 0;
}


// Retrieves the program name.
const char *cmdlineGetProgramName(void) {
	if (argCount == 0)
		return NULL;
	return args[0];
}


// Gets the corresponding value for a given option key ("-key=value" or
// "--key=value").
const char *cmdlineGetValueForKey(const char key[]) {
	if (key == NULL || argCount == 0)
		return NULL;
	key = skipLeadingDashes(key);
	size_t keyLen = strlen(key);
	if (keyLen == 0)
		return NULL;
	unsigned int i = argCount - 1;
	do {
		const char *arg = args[i];
		if (classify(arg) != KEY_VALUE_PAIR)
			continue;
		arg = skipLeadingDashes(arg);
		if (strncmp(arg, key, keyLen) != 0 || *(arg + keyLen) != '=')
			continue;
		return arg + keyLen + 1;
	} while (i-- > 0);
	return NULL;
}


// Checks if a given flag ("-flag" or "--flag") was passed via the command line.
bool cmdlineGetFlag(const char flag[]) {
	if (flag == NULL || argCount == 0)
		return false;
	flag = skipLeadingDashes(flag);
	for (unsigned int i = 1; i < argCount; ++i) {
		const char *arg = args[i];
		if (classify(arg) != FLAG)
			continue;
		arg = skipLeadingDashes(arg);
		if (strcmp(arg, flag) == 0)
			return true;
	}
	return false;
}


// Retrieves the number of extra command-line arguments.
unsigned int cmdlineGetExtraArgCount(void) {
	if (argCount == 0)
		return 0;
	unsigned int n = 0;
	for (unsigned int i = 1; i < argCount; ++i) {
		if (classify(args[i]) == EXTRA_ARG)
			++n;
	}
	return n;
}


// Retrieves an extra argument.
const char *cmdlineGetExtraArg(unsigned int index) {
	if (argCount == 0)
		return NULL;
	for (unsigned int i = 1; i < argCount; ++i) {
		if (classify(args[i]) != EXTRA_ARG)
			continue;
		if (index == 0)
			return args[i];
		--index;
	}
	return NULL;
}
