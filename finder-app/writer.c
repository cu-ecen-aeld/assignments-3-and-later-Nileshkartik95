#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	int fd = -1; 
	int wr_retval = -1;
	
	if((argc != 3) || (argv[1] == NULL) || (argv[2] == NULL))
	{
		openlog(NULL, 0 , LOG_USER);
		syslog(LOG_ERR," Invlaid number of arguments: %d",argc);
		return 1;
	}
	
	int buffer_len = strlen(argv[2]);
	fd = open(argv[1], O_WRONLY | O_CREAT | O_NONBLOCK, S_IRWXU | S_IRWXG | S_IRWXO);
	
	if(fd != -1)
	{
		syslog(LOG_DEBUG,"Writing %s to %s",argv[2],argv[1]);
		wr_retval = write(fd, argv[2], buffer_len);
		if((wr_retval == -1) || (wr_retval != buffer_len))
		{
			syslog(LOG_ERR," Write to file failed or insufficient space alloted ");
		}
	}
	else
	{
		syslog(LOG_ERR,"File failed to open");
	}
	close(fd);
	
	
	
}
