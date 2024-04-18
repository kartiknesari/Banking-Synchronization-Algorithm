#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../utilities.h"

HostInfo *readHostsFromFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        return NULL;
    }

    HostInfo *hosts = malloc(MAX_HOSTS * sizeof(HostInfo));
    if (!hosts)
    {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    char line[MAX_HOSTNAME_LENGTH + 10]; // Assuming the ID won't exceed 10 characters
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < MAX_HOSTS)
    {
        // Tokenize the line to separate hostname and ID
        char *temp_hostname = strtok(line, " ");
        size_t i = strcspn(temp_hostname, ".");
        char *hostname = strndup(temp_hostname, i);
        char *id_str = strtok(NULL, " \n");

        if (hostname && id_str)
        {
            // Store hostname
            strcpy(hosts[count].hostname, hostname);
            // Convert ID from string to integer
            hosts[count].id = atoi(id_str);
            count++;
        }
    }

    fclose(file);

    return hosts;
}

int get_host_id(const char *filename)
{
    static char hostname[MAX_HOSTNAME_LENGTH]; // Static buffer to hold the hostname
    if (gethostname(hostname, MAX_HOSTNAME_LENGTH) != 0)
    {
        // Error occurred while getting hostname
        perror("Error getting hostname");
        return -1;
    }
    HostInfo *hosts = readHostsFromFile(filename);
    if (!hosts)
    {
        printf("Failed to read hosts from file.\n");
        return -1;
    }
    // for (int i = 0; i < MAX_HOSTS; i++)
    //     printf("%s : %d\n", hosts[i].hostname, hosts[i].id);
    for (int i = 0; i < MAX_HOSTS; i++)
    {
        if (!strcmp(hosts[i].hostname, hostname))
            return hosts[i].id;
    }
    return -1;
}