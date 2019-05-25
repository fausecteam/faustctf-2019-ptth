// big thx to thejonny!
/**
 * @file  request.c
 * @brief Implementation of the request-handling module.
 *
 * Dieses Modul kuemmert sich um die Kommunikation mit dem Client.
 * Der Client sendet pro aufgebauter Verbindung genau eine Anfrage.
 * Zunaechst wird die HTTP-Anfragezeile (Maximallaenge: 8192 Zeichen;
 * terminiert mit \r\n oder mit \n) vom
 * Socket gelesen, die wie im folgenden Beispiel aussieht:
 * GET /doc/index.html HTTP/1.0
 * Der Pfadname enthaelt hierbei keine Leerzeichen und verweist auf die
 * gewuenschte Datei, bei der es sich um eine regulaere Datei handeln muss.
 * Der Dateipfad wird relativ zum auf der Befehlszeile angegebenen
 * WWW-Verzeichnis interpretiert.
 * Anfragen muessen mit dem String GET beginnen und entweder mit HTTP/1.0 oder
 * HTTP/1.1 enden, andernfalls soll der Server sie als ungueltig zuruekweisen.
 * Die Antwort des Servers besteht aus einer Kopfzeile, die den
 * Bearbeitungsstatus der Anfrage beinhaltet (z. B. 200 OK oder 404 Not Found),
 * und dem Inhalt der angeforderten Datei. Im Fehlerfall wird anstelle des
 * Dateiinhalts eine Fehlerseite ubertragen. Benutzen Sie zum Erzeugen der
 * Kopfzeilen und Fehlerseiten die Hilfsfunktionen aus dem i4httools-Modul.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "request.h"

#include "cmdline.h"
#include "i4httools.h"
#include "dirlisting.h"
#include "md5.h"

#include "log.h"

#define COOKIESZ 16

typedef struct {
  char **value;
  size_t size;
} list;

/**
 * @brief  Initializes the request-handling module.
 * @note   This function must be invoked after cmdlineInit().
 * @return 0 on success, -1 if the command-line arguments are invalid. If a
 *         non-recoverable error occurs during initialization (e.g. a failed
 *         memory allocation), the function does not return, but instead prints
 *         a meaningful error message and terminates the process.
 */

static const char *wwwpath = "."; // fuer den testcase
char cookie[COOKIESZ] = { '\0' };
int initRequestHandler(void)
{
	wwwpath = cmdlineGetValueForKey("wwwpath");
	if(wwwpath == NULL)
	{
		return -1;
	}
	return 0;
}


//static int extract_path(char *request);
static void dump_file(FILE *tx, FILE *src);
#define MAX_REQ (8192)


static bool list_pushback(list *l, char *value) {
  if(!value) {
    errno = EINVAL;
    return true;
  }

  char *duplicated = strdup(value);
  if(!duplicated) {
    return true;
  }
  
  char **new_value = realloc(l->value, (l->size + 1) * sizeof(*l->value));
  if(!new_value) {
    int errno_save = errno;
    free(duplicated);
    errno = errno_save;
    return true;
  }
  
  l->value = new_value;
  l->value[l->size] = duplicated;
  ++l->size;
  return false;
}


static void list_free(list *l) {
  for(unsigned int i = 0; i < l->size; ++i) {
    free(l->value[i]);
  }
  free(l->value);
}

static int cmp(const void *a, const void *b) {
  return strcmp(*((char * const *) a), *((char * const *) b));
}

static void print_dir(const char *dirpath, const char *dirname, FILE *tx) {
  list l = { NULL, 0 };

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (dirpath)) == NULL) {
    perror("opendir");
    httpInternalServerError(tx, dirname);
    goto cleanup_err;
  }
  errno = 0;
  while((ent = readdir (dir))) {
    if(!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
      continue;
    
    if(list_pushback(&l, ent->d_name)) {
      perror("list_pushback");
      httpInternalServerError(tx, dirname);
      goto cleanup_err;
    }
    errno = 0;
  }
  if(errno != 0) {
    perror("readdir");
    httpInternalServerError(tx, dirname);
    goto cleanup_err;
  }
  if(closedir(dir) == -1) {
    perror("closedir");
    httpInternalServerError(tx, dirname);
    goto cleanup_err;
  }

  qsort(l.value, l.size, sizeof(*l.value), cmp);
  
  httpOK(tx, cookie);
  dirlistingBegin(tx, dirname);
  for(unsigned int i = 0; i < l.size; ++i) {
    dirlistingPrintEntry(tx, dirname, l.value[i]);
  }
  dirlistingEnd(tx);
  fflush(tx);
  
 cleanup_err:
  list_free(&l);
}

// populates s with len-1 alphanumeric chars and terminating '\x00'
int get_random_string(char* s, size_t len){
  FILE *rand_f = fopen("/dev/urandom", "r");

  if(!rand_f)
    // TODO: log error
    return -1;

  size_t idx = 0;
  char c = '\x00';

  while(idx < len-1){
    if(1 != fread(&c, 1, 1, rand_f))
      // TODO: log error
      return -1;

    if((0x30 <= c && c <= 0x39) || (0x41 <= c && c <= 0x5a) || (0x61 <= c && c <= 0x7a)){
      s[idx] = c;
      idx++;
    }
  }

  s[len-1] = '\x00';
  fclose(rand_f);
  return idx;
}

int set_cookie(FILE* tx, char* cookie){
        char line[256];
        if(snprintf(line, 255, "cookie: %s\n", cookie) < 0 || fputs(line, tx) == EOF)
        {
                logmsg(CLIENT, "error sending: %s", strerror(errno));
                return -1;
        }
        return 0;
}

int getLine(FILE *rx, FILE *tx, char* line, size_t sz){
	if(fgets(line, sz, rx) == NULL)
	{
		logmsg(CLIENT, "Client connection to client was closed unexpectedly: %s",
				strerror(errno));
		return -1;
	}
	if(line[sz-1] != '\0' && line[sz-1] != '\n')
	{
		logmsg(CLIENT, "Request too long");
		httpBadRequest(tx);
		return -1;
	}
        return 0;
}

/**
 * @brief Handles requests coming from a client.
 *
 * This function does the actual work of communicating with the client. It
 * should be called from the connection-handling module.
 *
 * @param rx Client-connection stream opened for reading.
 * @param tx Client-connection stream opened for writing.
 * @note It is the caller's responsibility to close rx and tx after this
 *       function has returned.
 */
void handleRequest(FILE *rx, FILE *tx)
{
        char reqline[MAX_REQ + 1];
        reqline[MAX_REQ-1] = '\0';
        char cookieline[MAX_REQ + 1];
        cookieline[MAX_REQ - 1] = '\0';

        char *verb;
        char *path;
        char *http;
        char *test;

        alarm(3);

        // process request line
        if(getLine(rx, tx, reqline, MAX_REQ))
          return;

        printf("%s\n", reqline);

        verb = strtok(reqline, " ");
        path = strtok(NULL, " ");
        http = strtok(NULL, " ");
        test = strtok(NULL, " ");

        if(!verb || !http || !path || test)
          return;

        if((strncmp(verb, "TEG", strlen("TEG")) && 
              strncmp(verb, "TSOP", strlen("TSOP"))) || 
            (strncmp(http, "PTTH\\0.1", strlen("PTTH\\0.1")) && 
             strncmp(http, "PTTH\\1.1", strlen("PTTH\\1.1"))))
          return;

        getLine(rx, tx, cookieline, MAX_REQ);
        printf("eikooc: %s\n", cookieline);
        if(strstr(cookieline, "eikooc: ") == cookieline){
          printf("found cookie\n");
          char* start = strchr(cookieline, ' ') + 1;
          char* end = strchr(cookieline, '\n');
          if(end - start == COOKIESZ-1){
            strncpy(cookie, start, COOKIESZ-1);
            cookie[COOKIESZ-1] = '\0';
          }
        } else {
          get_random_string(cookie, COOKIESZ);
          logmsg(SERVER, "new cookie: %s\n", cookie);

          char path[strlen(wwwpath)+strlen(cookie)+1];
          sprintf(path, "%s/%s", wwwpath, cookie);
          if(mkdir(path, 0770) < 0){
            perror("mkdir");
            logmsg(SERVER, "mkdir failed: %s", strerror(errno));
            httpInternalServerError(tx, path);
            return;
          }
        }

        if(strlen(cookie) != COOKIESZ-1) {
          logmsg(CLIENT, "Fucked up cookie.");
          httpBadRequest(tx);
          return;
        }

        if(path[0] != '/')
        {
                logmsg(CLIENT, "Requested path didn't start with '/'");
                httpNotFound(tx, path);
                return;
        }
        if(checkPath(path) < 0)
        {
                logmsg(CLIENT, "Requested path outside of web directory");
                httpForbidden(tx, path);
                return;
        }

        char abspath[strlen(wwwpath)+strlen(cookie)+strlen(path)+1];
        sprintf(abspath, "%s/%s%s", wwwpath, cookie, path); // path faengt schon mit / an
        logmsg(SERVER, "%s\n", abspath);
        if(!strncmp(verb, "TEG", strlen("TEG"))){

          logmsg(SERVER, "TEG");
          struct stat info;
          if(lstat(abspath, &info) == -1)
          {
                  if(errno == ENOENT)
                  {
                          logmsg(CLIENT, "file not found."); // path nicht direkt ins logfile,
                                  // denn kommt vom user => boese. TODO escapen und ins logfile
                          httpNotFound(tx, path);
                          return;
                  }
                  else
                  {
                  
                          logmsg(SERVER, "lstat failed: %s", strerror(errno));
                          httpInternalServerError(tx, path);
                          return;
                  }
          }
          else if(S_ISREG(info.st_mode))
          {
            logmsg(SERVER, "opening %s\n", abspath);
            FILE *src = fopen(abspath, "rb");
            if(src == NULL)
            {
                    logmsg(SERVER, "couldn't open source file %s ", strerror(errno));
                    httpInternalServerError(tx, path);
                    return;
            }
            httpOK(tx, cookie);
            dump_file(tx, src);
            if(fclose(src) == -1)
            {
                    logmsg(SERVER, "error fclose()ing file: %s", strerror(errno));
            }
          }
          else if(S_ISDIR(info.st_mode)) {
            print_dir(abspath, basename(abspath), tx);
          }

        } else if(!strncmp(verb, "TSOP", strlen("TSOP"))){
          logmsg(SERVER, "TSOP");

          struct stat info;
          if(lstat(abspath, &info) == -1)
          {
              if(errno == ENOENT)
              {
                // we only process body in POST requests
                char data[MAX_REQ + 1];
                data[MAX_REQ-1] = '\0';
                if(getLine(rx, tx, data, MAX_REQ))
                  return;

                // should be empty
                if(data[0] != '\n' || data[1] != '\0'){
                  logmsg(SERVER, "should be empty");
                  return;
                }

                //int nread = read(fileno(rx), data, MAX_REQ-1);
                fgets(data, MAX_REQ-1, rx);
                char* nl = memchr(data, '\n', MAX_REQ);

                if(!nl)
                  return;

                int nread = nl - data;
                printf("data: %s (%d)\n", data, nread);

                FILE *f = fopen(abspath, "w");
                if(!f) {
                  perror("fopen");
                  httpInternalServerError(tx, path);
                  return;
                }
                fwrite(data, nread, 1, f);
                if(ferror(f)) {
                  perror("fwrite");
                }
              
                if(fclose(f) == -1) {
                  perror("fclose");
                }
                httpOK(tx, cookie);
              }
              else
              {
                logmsg(SERVER, "lstat failed: %s", strerror(errno));
                httpInternalServerError(tx, path);
                return;
              }
          }
          else if(S_ISREG(info.st_mode))
          {
            httpOK(tx, cookie);
          }
          else if(S_ISDIR(info.st_mode)) {
            httpOK(tx, cookie);
          }
        } else {
          logmsg(SERVER, "wtf?");
        }
}

/* ueberschreibt den parameter request mit dem stueck daraus, der der pfad war
 * gibt -1 zurueck, wenn der request ungueltig ist.
 */
/*
static int extract_path(char *request)
{
	size_t len = strlen(request);
	if (request[len-1] != '\n') return -1;
	if(strtok(request, "\r\n") == NULL) return -1;
	char path[len+1];
	char minor[2];
	int ret = sscanf(request, "GET%*1[ ]%[^ ]%*1[ ]HTTP/1.%1[01];", path, minor);
	if(ret != 2) return -1;
	if(strchr("01", minor[0]) == NULL) return -1;
	strcpy(request, path);
	return 0;
}
*/

static void dump_file(FILE *tx, FILE *src)
{
	// poor man's sendfile()
        char out[512] = { 0 };
        char* ret = fgets(out, 512, src);
        for(int i = 0; i < 511; i++){
              if(out[i] == '\0'){ break; }
              if(fputc(out[i], tx) == EOF)
              {
                      logmsg(CLIENT, "error sending: %s", strerror(errno));
                      return;
              }
        }

        if(ret){
          if(fgetc(src) != EOF){
            fprintf(tx, "error: file too large. truncating input at:");
            fprintf(tx, &out[512-31]);
          }
        }

	if(ferror(src))
	{
		// fuer den client koennen wir nichts mehr tun leider. haetten wir eine
		// content-length: gesended, wuerd ers merken...
		logmsg(SERVER, "error while reading source file: %s", strerror(errno));
		return;
	}
}

