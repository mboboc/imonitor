/*
 * Useful functions
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "imonitor.h"

int is_req_finished(char *buffer, int offset)
{
	if (buffer[offset - 1] != '\n' && buffer[offset - 2] != '\r' &&
		buffer[offset - 3] != '\n' && buffer[offset - 4] != '\r')
		return 1;
	return 0;
}

void write_http200_header(char *buffer, int content_len)
{
	sprintf(buffer,
		"HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: \
		application/octet-stream\r\nConnection: close\r\n\r\n",
		content_len);
}

void write_http404_header(char *buffer)
{
	sprintf(buffer, "HTTP/1.1 404 Not Found\r\n\r\n");
}

int is_file(char *buffer)
{
	if (access(buffer + 1, F_OK) != -1)
		return 1;
	return 0;
}
