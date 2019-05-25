/**
 * @file  i4httools.h
 * @brief Miscellaneous routines for building an HTTP server.
 *
 * This module contains some routines that are useful for implementing a web
 * server.
 */

#ifndef I4HTTOOLS_H
#define I4HTTOOLS_H

#include <stdio.h>


/**
 * @brief Checks if a path leaves the webserver's document-root (wwwpath).
 *
 * This routine checks if the path (as obtained from the HTTP GET request)
 * ascends the VFS beyond the webserver's document-root directory (wwwpath).
 * Such requests should not be served.
 *
 * @note   This function stops processing as soon as the path ascends beyond
 *         the root of the web directory.
 * @param  path Requested URL.
 * @return Positive depth of the request relative to the web directory,
 *         or -1 if the relative URL ascends beyond the web directory.
 */
int checkPath(const char path[]);

/**
 * @brief Outputs an "OK" HTTP response to a stream.
 *
 * This routine generates an "OK (200)" status line on the given stream, which
 * should normally be the client connection.
 *
 * @param tx The output stream.
 */
void httpOK(FILE *tx, char* cookie);

/**
 * @brief Outputs a "moved permanently" HTTP response and error page to a
 *        stream.
 *
 * This routine generates a "moved permanently (301)" HTTP response on the given
 * stream, which should normally be the client connection.
 *
 * @param tx         The output stream.
 * @param newRelPath Path to which the request is redirected.
 *
 * @note This function is not needed for assignment 2 (@c sister), only for
 *       assignment 5 (@c mother).
 */
void httpMovedPermanently(FILE *tx, const char newRelPath[]);

/**
 * @brief Outputs a "bad request" HTTP response and error page to a stream.
 *
 * This routine generates an "HTTP/1.0 400 Bad Request" status line and a
 * corresponding HTML error page on the given stream, which should normally be
 * the client connection.
 *
 * @param tx The output stream.
 */
void httpBadRequest(FILE *tx);

/**
 * @brief Outputs a "forbidden" HTTP response and error page to a stream.
 *
 * This routine generates an "HTTP/1.0 403 Forbidden" status line and a
 * corresponding HTML error page on the given stream, which should normally be
 * the client connection.
 *
 * @param tx      The output stream.
 * @param relPath Requested URL.
 */
void httpForbidden(FILE *tx, const char relPath[]);

/**
 * @brief Outputs a "not found" HTTP response and error page to a stream.
 *
 * This routine generates an "HTTP/1.0 404 Not Found" status line and a
 * corresponding HTML error page on the given stream, which should normally be
 * the client connection.
 *
 * @param tx      The output stream.
 * @param relPath Requested URL.
 */
void httpNotFound(FILE *tx, const char relPath[]);

/**
 * @brief Outputs an "internal server error" HTTP response and error page to a
 *        stream.
 *
 * This routine generates an "HTTP/1.0 500 Internal Server Error" status line
 * and a corresponding HTML error page on the given stream, which should
 * normally be the client connection.
 *
 * @param tx      The output stream.
 * @param relPath Requested URL. May be @c NULL if not applicable.
 */
void httpInternalServerError(FILE *tx, const char relPath[]);

#endif /* I4HTTOOLS_H */
