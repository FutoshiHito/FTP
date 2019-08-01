#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>

inline void error(const char *msg)
{
	perror(msg);
	exit(0);
}

inline void print(const char *msg)
{
	printf(msg);
	fflush(stdout);
}

inline void println(const char *msg)
{
	printf("%s\n", msg);
	fflush(stdout);
}

#endif