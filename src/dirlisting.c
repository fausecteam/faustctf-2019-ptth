#include "dirlisting.h"
#include <libgen.h>
#include <string.h>


static void doPrintEntry(FILE *stream, const char dirName[],
                         const char entryName[], const char label[]) {
	size_t dirLen = strlen(dirName);
	if (dirLen > 0 && dirName[dirLen - 1] == '/')
		fprintf(stream, "<a href=\"%s%s\">%s</a><br>\n", dirName, entryName,
		        label);
	else
		fprintf(stream, "<a href=\"%s/%s\">%s</a><br>\n", dirName, entryName,
		        label);
}


// Prints the headline of a directory-listing HTML page to a stream.
void dirlistingBegin(FILE *stream, const char dirName[]) {

	// Print HTML header
	fprintf(stream, "<html><head>\n");
	fprintf(stream, "<title>Index of %s</title>\n", dirName);
	fprintf(stream, "</head><body>\n");
	fprintf(stream, "<h1>Index of %s</h1>\n", dirName);

	// Print parent directory    
	char dirCopy[strlen(dirName) + 1];
	strcpy(dirCopy, dirName);
	doPrintEntry(stream, dirname(dirCopy), "", "(Parent Directory)");
}


// Prints an entry of a directory listing to a stream.
void dirlistingPrintEntry(FILE *stream, const char dirName[],
                          const char entryName[]) {
	doPrintEntry(stream, dirName, entryName, entryName);
}


void dirlistingEnd(FILE *stream) {
	fputs("</body></html>", stream);
}
