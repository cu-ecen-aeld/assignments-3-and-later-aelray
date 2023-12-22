#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
    openlog(NULL, 0, LOG_USER);
    atexit(closelog);
    if (argc < 3)
    {
        syslog(LOG_ERR, "Invalid Number of arguments: %d", argc);
        exit(EXIT_FAILURE);
    }
    FILE * pFile;
    pFile = fopen (argv[1],"w");
    if (pFile!=NULL)
    {
        syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
        fputs (argv[2],pFile);
        fclose (pFile);
    }
    else
    {
        syslog(LOG_ERR, "Couldn't open file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
}