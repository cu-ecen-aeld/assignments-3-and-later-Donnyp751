#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define IDENTITY "writer-app"
#define MAX_STR 127

int main(int argc, char** argv)
{
	int error = 0;
	size_t string_length = 0;
	char *p_string = NULL;
	char *p_filename = NULL;

	openlog(IDENTITY, 0, LOG_USER);

	if (argc != 3)
	{
		fprintf(stderr, "Invalid arguments, call format is ./writer FILENAME SEARCHSTR\n");
		return 1;
	}

	p_filename = argv[1];
	p_string = argv[2];

	int f_dst_file = open(p_filename, O_CREAT | O_WRONLY, 0444);

	if (!f_dst_file)
	{
		error = errno;
		syslog(LOG_ERR, "Failed to create file with error %d\n", error);
		fprintf(stderr, "Failed to create file with error %d\n", error);
		return 1; 
	}
	

	string_length = strnlen(p_string, MAX_STR); 

	syslog(LOG_DEBUG, "Writing %s to %s\n", p_filename, p_string);
	size_t write_size = write(f_dst_file, p_string, string_length);
	
	if (write_size != string_length)
	{
		error = errno;
		syslog(LOG_ERR, "Failed to write string to file with error %d\n", error);
		fprintf(stderr, "Failed to write string to file with error %d\n", error);
		return 1;
	}
	close(f_dst_file);

	return 0;
}
