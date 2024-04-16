#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "myudp.h"
#include "utilities.h"
#include "udp_procedures.h"

/* Global Declaration Start*/
// Thread declaration
pthread_t listener_tid;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// Socket Declaration
int cc;
int socket_fd;
HostInfo *hosts;
int host_id;
socklen_t fsize;
struct sockaddr_in s_in, from;
struct msg_packet msg;
// Account declaration
long account_balance;
/* Global Declaration end*/

void *listener_thread(void *arg);
int distributed_mutex_init();
void printsin(struct sockaddr_in *sin, char *m1, char *m2);
void *listener_thread(void *arg);

int main()
{
    if (distributed_mutex_init() == 0)
    {
        for (;;)
        {
            fsize = sizeof(from);
            cc = recvfrom(socket_fd, &msg, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, &fsize);
            if (cc < 0)
                perror("recv_udp:recvfrom");
            printsin(&from, "recv_udp: ", "Packet from:");
            printf("Got message :: hostID=%d: cmd=%d, seq=%d, tiebreak=%d\n", ntohl(msg.hostid), ntohs(msg.cmd), ntohs(msg.seq), ntohl(msg.tiebreak));

            fflush(stdout);
            return (0);
        }
    }
    else
    {
        perror("Distributed_mutex_init Error\n");
        return 0;
    }
}

int distributed_mutex_init()
{
    const char *filename = "./process.hosts";
    // feed process.hosts content to stuct hosts
    hosts = readHostsFromFile(filename);
    host_id = get_host_id(filename);
    if (host_id < 0 || host_id >= MAX_HOSTS)
    {
        perror("Host does not exist in process.hosts file\n");
        return -1;
    }
    printf("host id: %d\n", host_id);

    // creating socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        perror("recv_udp:socket creation error");
        return -1;
    }
    bzero((char *)&s_in, sizeof(s_in)); /* They say you must do this    */

    s_in.sin_family = (short)AF_INET;
    s_in.sin_addr.s_addr = htonl(INADDR_ANY); /* WILDCARD */
    s_in.sin_port = htons((unsigned short)0x3333);
    printsin(&s_in, "RECV_UDP", "Local socket is:");
    fflush(stdout);
    if (bind(socket_fd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0)
    {
        perror("recv_udp:bind socket error");
        return -1;
    }
    // initialize account
    account_balance = 0;

    pthread_create(&listener_tid, NULL, listener_thread, NULL);

    return 0;
}

void *listener_thread(void *arg)
{
    struct sockaddr_in from;
    socklen_t fsize;
    struct msg_packet msg;

    // Listener thread logic
    printf("Creating listener thread\n");
    while (1)
    {
        fsize = sizeof(from);
        int cc = recvfrom(socket_fd, &msg, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, &fsize);
        if (cc < 0)
        {
            perror("recvfrom");
            continue;
        }

        // Process received message
        // pthread_mutex_lock(&mutex);
        printsin(&from, "recv_udp: ", "Packet from:");
        printf("Got message :: hostID=%d: cmd=%d, seq=%d, tiebreak=%d\n",
               ntohl(msg.hostid), ntohs(msg.cmd), ntohs(msg.seq), ntohl(msg.tiebreak));
        // Update data structures, handle synchronization, etc.
        // pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void printsin(struct sockaddr_in *sin, char *m1, char *m2)
{

    printf("%s %s:\n", m1, m2);
    printf("  family %d, addr %s, port %d\n", sin->sin_family,
           inet_ntoa(sin->sin_addr), ntohs((unsigned short)(sin->sin_port)));
}