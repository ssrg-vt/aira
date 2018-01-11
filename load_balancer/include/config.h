/*
 * Compile-time flags for configuring behavior and printing information for
 * the client & server.
 *
 * Author: Rob Lyerly
 * Date: 8/8/2015
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/* Vomit debugging information to the screen */
//#define _VERBOSE 1

/* Vomit debugging information either from the client- and/or server-side */
#ifdef _VERBOSE
# define _SERVER_VERBOSE 1
# define _CLIENT_VERBOSE 1
#else
//# define _SERVER_VERBOSE 1
//# define _CLIENT_VERBOSE 1
#endif

/* Record & print statistics */
//#define _STATISTICS 1

/* Record & print statistics from either the client and/or server */
#ifdef _STATISTICS
# define _SERVER_STATISTICS 1
# define _CLIENT_STATISTICS 1
#else
# define _SERVER_STATISTICS 1
//# define _CLIENT_STATISTICS 1
#endif

#endif /* _CONFIG_H */

