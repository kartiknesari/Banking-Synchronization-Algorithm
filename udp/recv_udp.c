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
#include "myudp.h"
#include "utilities.h"
#include "udp_procedures.h"

int socket_fd;
HostInfo *hosts;
int host_id;

void printsin(struct sockaddr_in *sin, char *m1, char *m2)
{

    printf("%s %s:\n", m1, m2);
    printf("  family %d, addr %s, port %d\n", sin->sin_family,
           inet_ntoa(sin->sin_addr), ntohs((unsigned short)(sin->sin_port)));
}

void send_hello(int socket_fd, struct sockaddr_in *dest)
{
    struct msg_packet mymsg;
    mymsg.cmd = htons(HELLO);
    mymsg.seq = htons(1);
    mymsg.hostid = htonl(0); // Assuming this server's ID is 0
    mymsg.tiebreak = htonl(getpid());
    mymsg.vtime[0] = htons(0);
    mymsg.vtime[1] = htons(0);
    mymsg.vtime[2] = htons(0);
    mymsg.vtime[3] = htons(0);
    mymsg.vtime[4] = htons(0);

    sendto(socket_fd, &mymsg, sizeof(struct msg_packet), 0, (struct sockaddr *)dest, sizeof(*dest));
}

void broadcast_hello(int socket_fd, HostInfo *hosts, int host_id)
{
    for (int i = 0; i < MAX_HOSTS; i++)
    {
        if (host_id != hosts[i].id)
        {
            char *temp_hostname = (char *)malloc(strlen(hosts[i].hostname) + strlen(".eecs.csuohio.edu") + 1);
            if (temp_hostname == NULL)
            {
                perror("Memory allocation failed");
                exit(1);
            }
            strcpy(temp_hostname, hosts[i].hostname);
            strcat(temp_hostname, ".eecs.csuohio.edu");
            send_udp_message(socket_fd, host_id, "HELLO", temp_hostname);
            printf("Sent HELLO message to %s\n", temp_hostname);
            free(temp_hostname);
        }
    }
}

void sig_alrm(int signo)
{
    broadcast_hello(socket_fd, hosts, host_id);
}

int main()
{
    int cc;
    socklen_t fsize;
    struct sockaddr_in s_in, from;
    struct msg_packet msg;

    /*
        returns host id w.r.t process.hosts
    */
    const char *filename = "./process.hosts";
    hosts = readHostsFromFile(filename);
    host_id = get_host_id(filename);
    if (host_id < 0 || host_id > 4)
    {
        perror("Host does not exist in process.hosts file\n");
        exit(1);
    }
    printf("host id: %d\n", host_id);

    /*
       Create the socket to be used for datagram reception. Initially,
       it has no name in the internet (or any other) domain.
    */
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        perror("recv_udp:socket");
        exit(1);
    }

    /*
       In order to attach a name to the socket created above, first fill
       in the appropriate blanks in an inet socket address data structure
       called "s_in". We blindly pick port number 0x3333. The second step
       is to BIND the address to the socket. If port 0x3333 is in use, the
       bind system call will fail detectably.
    */

    bzero((char *)&s_in, sizeof(s_in)); /* They say you must do this    */

    s_in.sin_family = (short)AF_INET;
    s_in.sin_addr.s_addr = htonl(INADDR_ANY); /* WILDCARD */
    s_in.sin_port = htons((unsigned short)0x3333);
    printsin(&s_in, "RECV_UDP", "Local socket is:");
    fflush(stdout);

    /*
       bind port 0x3333 on the current host to the socket accessed through
       socket_fd. If port in use, die.
    */
    if (bind(socket_fd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0)
    {
        perror("recv_udp:bind");
        exit(1);
    }
    if (host_id == 0)
    {
        for (int i = 0; i < MAX_HOSTS; i++)
        {
            if (host_id != hosts[i].id)
            {
                char *temp_hostname = (char *)malloc(strlen(hosts[i].hostname) + strlen(".eecs.csuohio.edu") + 1);
                if (temp_hostname == NULL)
                {
                    perror("Memory allocation failed");
                    exit(1);
                }
                strcpy(temp_hostname, hosts[i].hostname);
                strcat(temp_hostname, ".eecs.csuohio.edu");
                send_udp_message(socket_fd, host_id, "HELLO", temp_hostname);
                printf("Sent HELLO message to %s\n", temp_hostname);
                free(temp_hostname);
            }
        }
    }
    for (;;)
    {
        fsize = sizeof(from);
        cc = recvfrom(socket_fd, &msg, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, &fsize);
        if (cc < 0)
            perror("recv_udp:recvfrom");
        printsin(&from, "recv_udp: ", "Packet from:");
        printf("Got message :: hostID=%d: cmd=%d, seq=%d, tiebreak=%d\n", ntohl(msg.hostid), ntohs(msg.cmd), ntohs(msg.seq), ntohl(msg.tiebreak));

        if (ntohs(msg.cmd) == HELLO)
        {
            struct msg_packet reply;
            reply.cmd = htons(HELLO_REPLY);
            reply.seq = htons(1);
            reply.hostid = htonl(host_id);
            reply.tiebreak = htonl(getpid());
            reply.vtime[0] = htons(0);
            reply.vtime[1] = htons(0);
            reply.vtime[2] = htons(0);
            reply.vtime[3] = htons(0);
            reply.vtime[4] = htons(0);

            sendto(socket_fd, &reply, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, sizeof(from));
            printf("Sent HELLO_REPLY message to %s\n", inet_ntoa(from.sin_addr));

            int alrm_time = 0;
            switch (host_id)
            {
            case 1:
                alrm_time = 2;
                break;
            case 2:
                alrm_time = 3;
                break;
            case 3:
                alrm_time = 5;
                break;
            case 4:
                alrm_time = 7;
                break;
            default:
                break;
            }
            signal(SIGALRM, sig_alrm);
            alarm(alrm_time); // set the timer
        }
        fflush(stdout);
    }
    return (0);
}
