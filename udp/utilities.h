#ifndef UTILITIES_H
#define UTILITIES_H

#define MAX_HOSTS 5
#define MAX_HOSTNAME_LENGTH 256

#define UDP_PORT 0x6874

typedef struct
{
    char hostname[MAX_HOSTNAME_LENGTH];
    int id;
} HostInfo;

HostInfo *readHostsFromFile(const char *filename);
int get_host_id(const char *filename);

#endif