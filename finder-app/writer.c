#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
    int ret = 1;
    openlog(NULL, LOG_PID | LOG_NDELAY, LOG_USER);
    if (argc < 3)
    {
        syslog(LOG_ERR, "Usage: %s <filename> <string>", argv[0]);
        goto exit;
    }

    const char *filename = argv[1];
    const char *write_str = argv[2];

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if(fd == -1)
    {
        syslog(LOG_ERR, "ERROR ocurred while opening the file: %s", filename);
        goto exit;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", write_str, filename);
    int bytes = write(fd, write_str, strlen(write_str));
    if(bytes == -1)
    {
        syslog(LOG_ERR, "ERROR ocurred while writng to %s", filename);
        goto cleanup;
    }
    else if (bytes != strlen(write_str))
    {
        syslog(LOG_ERR, "ERROR partial write");
        goto cleanup;
    }
    ret = 0;

cleanup:
    close(fd);
exit:
    closelog();
    return ret;
}