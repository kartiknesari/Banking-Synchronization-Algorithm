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
#include <stdbool.h>

unsigned int account_balance;

int socket_fd;
HostInfo *hosts;
int host_id;
pthread_t listen_thread;

int cc;
socklen_t fsize;
struct sockaddr_in s_in, from;
struct msg_packet msg;

int RD[MAX_HOSTS];
int msg_replies;
int vector_clock[5];
bool enter_critical;
bool request_flag;

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

void broadcast_request(int socket_fd, HostInfo *hosts, int host_id, struct msg_packet msg)
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
            send_udp_message(socket_fd, host_id, "HELLO", temp_hostname, msg);
            // printf("Sent HELLO message to %s\n", temp_hostname);
            free(temp_hostname);
        }
    }
}

void sig_alrm(int signo)
{
    msg.cmd = htons(HELLO);
    msg.seq = htons(1);
    msg.hostid = htonl(host_id);
    msg.tiebreak = htonl(getpid());
    msg.vtime[0] = htons(0);
    msg.vtime[1] = htons(0);
    msg.vtime[2] = htons(0);
    msg.vtime[3] = htons(0);
    msg.vtime[4] = htons(0);
    broadcast_request(socket_fd, hosts, host_id, msg);
}

void *message_listener(void *arg)
{
    int cc;
    socklen_t fsize;
    struct sockaddr_in from;
    struct msg_packet received_msg;

    for (;;)
    {
        fsize = sizeof(from);
        cc = recvfrom(socket_fd, &received_msg, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, &fsize);
        if (cc < 0)
        {
            perror("recv_udp:recvfrom");
        }
        else
        {
            printsin(&from, "recv_udp: ", "Packet from:");
            printf("Got message :: hostID=%d: cmd=%d, seq=%d, tiebreak=%d\n", ntohl(received_msg.hostid), ntohs(received_msg.cmd), ntohs(received_msg.seq), ntohl(received_msg.tiebreak));

            if (ntohs(received_msg.cmd) == REPLY)
            {
                printf("Received REPLY message from hostID=%d\n", ntohl(received_msg.hostid));
                msg_replies++;
            }
            if (ntohs(received_msg.cmd) == REQUEST)
            {
                // printf("Received REQUEST message from hostID=%d\n", ntohl(received_msg.hostid));
                int requesting_host_id = ntohl(received_msg.hostid);
                int requesting_pid = ntohl(received_msg.tiebreak);
                int requesting_timestamp = ntohs(received_msg.vtime[requesting_host_id]);

                // Check if site Sj is neither requesting nor executing the CS
                // or if Sj is requesting and Si's request's timestamp is smaller
                // or if the timestamps are concurrent but Si's PID is smaller
                printf("Requesting Host ID: %d\n", requesting_host_id);
                for (int i = 0; i < MAX_HOSTS; i++)
                    printf("%hu ", ntohs(received_msg.vtime[i]));
                printf("\n");
                printf("Host ID: %d\n", host_id);
                for (int i = 0; i < MAX_HOSTS; i++)
                    printf("%d ", vector_clock[i]);
                printf("\n");
                // bool can_send_reply = (!RD[host_id] && (requesting_timestamp < vector_clock[host_id])) || ((requesting_timestamp == vector_clock[host_id] && requesting_pid < ntohl(msg.tiebreak)));

                bool can_send_reply = false;

                if (!enter_critical)
                // check if entered critical
                {
                    if (request_flag)
                    // check if request message sent
                    {
                        if (requesting_timestamp < vector_clock[host_id])
                            // check if requester timestamp less than host timestamp
                            can_send_reply = true;
                        else if (requesting_timestamp == vector_clock[host_id])
                        {
                            // if requester timestamp equals host timestamp
                            if (requesting_pid > msg.tiebreak)
                                can_send_reply = true;
                            else
                                can_send_reply = false;
                        }
                        else
                            can_send_reply = false;
                    }
                    else
                    // host has not sent request message yet
                    {
                        can_send_reply = true;
                    }
                }

                if (can_send_reply)
                {
                    // Send a REPLY message to site Si
                    struct msg_packet reply;
                    reply.cmd = htons(REPLY);
                    reply.seq = htons(1);
                    reply.hostid = htonl(host_id);
                    reply.tiebreak = htonl(getpid());
                    for (int i = 0; i < MAX_HOSTS; i++)
                    {
                        if (vector_clock[i] < received_msg.vtime[i])
                            vector_clock[i] = received_msg.vtime[i];
                    }
                    for (int i = 0; i < MAX_HOSTS; i++)
                        reply.vtime[i] = htons(vector_clock[i]);
                    // memcpy(reply.vtime, vector_clock, sizeof(msg.vtime));
                    // msg.account_balance = reply.account_balance;

                    sendto(socket_fd, &reply, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, sizeof(from));
                    printf("Sent REPLY message to hostID=%d\n", requesting_host_id);
                }
                else
                {
                    // Defer the reply and set RD[i] = 1
                    printf("Deferring the reply\n");
                    RD[requesting_host_id] = 1;
                    printf("Deferred Array: ");
                    for (int i = 0; i < MAX_HOSTS; i++)
                        printf("%d ", RD[i]);
                    printf("\n");
                }
            }

            if (ntohs(received_msg.cmd) == UPDATE)
            {
                account_balance = ntohl(received_msg.account_balance);
            }

            // if (ntohs(msg.cmd) == HELLO)
            // {
            //     struct msg_packet reply;
            //     reply.cmd = htons(HELLO_REPLY);
            //     reply.seq = htons(1);
            //     reply.hostid = htonl(host_id);
            //     reply.tiebreak = htonl(getpid());
            //     reply.vtime[0] = htons(0);
            //     reply.vtime[1] = htons(0);
            //     reply.vtime[2] = htons(0);
            //     reply.vtime[3] = htons(0);
            //     reply.vtime[4] = htons(0);

            //     sendto(socket_fd, &reply, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, sizeof(from));
            //     printf("Sent HELLO_REPLY message to %s\n", inet_ntoa(from.sin_addr));

            //     int alrm_time = 0;
            //     switch (host_id)
            //     {
            //     case 1:
            //         alrm_time = 2;
            //         break;
            //     case 2:
            //         alrm_time = 3;
            //         break;
            //     case 3:
            //         alrm_time = 5;
            //         break;
            //     case 4:
            //         alrm_time = 7;
            //         break;
            //     default:
            //         break;
            //     }
            //     signal(SIGALRM, sig_alrm);
            //     alarm(alrm_time); // set the timer
            // }
            fflush(stdout);
        }
    }

    return NULL;
}

int distributed_mutex_init()
{
    int result = 0;

    // Perform necessary variable initializations here
    account_balance = 100;
    msg_replies = 0;
    vector_clock[0] = 0;
    vector_clock[1] = 0;
    vector_clock[2] = 0;
    vector_clock[3] = 0;
    vector_clock[4] = 0;

    request_flag = false;
    enter_critical = false;
    // Create the socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        perror("recv_udp:socket");
        result = -1;
    }
    else
    {
        bzero((char *)&s_in, sizeof(s_in)); /* They say you must do this    */

        s_in.sin_family = (short)AF_INET;
        s_in.sin_addr.s_addr = htonl(INADDR_ANY); /* WILDCARD */
        s_in.sin_port = htons((unsigned short)UDP_PORT);
        printsin(&s_in, "RECV_UDP", "Local socket is:");
        fflush(stdout);

        // Bind the socket to the specified port
        if (bind(socket_fd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0)
        {
            perror("recv_udp:bind");
            result = -1;
        }
        else
        {
            // Create the pthread for listening to incoming messages
            if (pthread_create(&listen_thread, NULL, message_listener, NULL) != 0)
            {
                perror("Error creating pthread");
                result = -1;
            }
        }
    }

    return result;
}

int distributed_mutex_lock()
{
    // Step 1: Broadcast a timestamped REQUEST message to all other processes
    msg.cmd = htons(REQUEST);
    msg.seq = htons(1);
    msg.hostid = htonl(host_id); // Assuming this server's ID is 0
    msg.tiebreak = htonl(getpid());
    vector_clock[host_id] += 1;
    for (int i = 0; i < MAX_HOSTS; i++)
        msg.vtime[i] = htons(vector_clock[i]);
    msg.account_balance = htonl(account_balance);

    request_flag = true;

    broadcast_request(socket_fd, hosts, host_id, msg);

    // Step 2: Wait for REPLY messages from all other processes
    while (msg_replies < MAX_HOSTS - 1)
        ;

    printf("Entering critical section\n");
    enter_critical = true;
    request_flag = false;
    return 0;
}

int distributed_mutex_unlock()
{
    // Iterate through the RD array and send deferred REPLY messages
    struct msg_packet reply;
    for (int j = 0; j < MAX_HOSTS; j++)
    {
        if (RD[j] == 1)
        {
            // Send a REPLY message to host j
            reply.cmd = htons(REPLY);
            reply.seq = htons(1);
            reply.hostid = htonl(host_id);
            reply.tiebreak = htonl(getpid());
            for (int i = 0; i < MAX_HOSTS; i++)
                reply.vtime[i] = htons(vector_clock[i]);
            reply.account_balance = htonl(account_balance);
            // You may need to update the timestamp in the reply message according to your implementation

            struct sockaddr_in dest;
            memset(&dest, 0, sizeof(dest));
            dest.sin_family = AF_INET;
            dest.sin_port = htons(UDP_PORT);
            inet_pton(AF_INET, hosts[j].hostname, &dest.sin_addr);

            sendto(socket_fd, &reply, sizeof(struct msg_packet), 0, (struct sockaddr *)&dest, sizeof(dest));
            printf("Sent deferred REPLY message to hostID=%d\n", j);

            // Reset the RD[j] flag
            RD[j] = 0;
        }
    }
    // broadcast update
    reply.cmd = htons(UPDATE);
    reply.seq = htons(1);
    reply.hostid = htonl(host_id);
    reply.tiebreak = htonl(getpid());
    for (int i = 0; i < MAX_HOSTS; i++)
        reply.vtime[i] = htons(vector_clock[i]);
    reply.account_balance = htonl(account_balance);
    broadcast_request(socket_fd, hosts, host_id, reply);

    enter_critical = false;
    printf("Out of Critical Section\n");
    return 0;
}

int main()
{
    // Read host information from the file and get the host ID
    const char *filename = "./process.hosts";
    hosts = readHostsFromFile(filename);
    host_id = get_host_id(filename);
    if (host_id < 0 || host_id > 4)
    {
        perror("Host does not exist in process.hosts file\n");
        return 1;
    }
    printf("host id: %d\n", host_id);

    // Initialize the distributed mutex

    if (!distributed_mutex_init())
    {
        int entry_flag = 0;
        char demo[5];
        while (entry_flag == 0)
        {
            printf("Do you want to start the experiment? (Yes | No) : ");
            scanf("%s", demo);
            if (!strcmp(demo, "Yes") || !strcmp(demo, "yes") || !strcmp(demo, "Y") || !strcmp(demo, "y"))
            {

                entry_flag = 1;
            }
        }
        for (int i = 0; i < 20; i++)
        {
            if (!distributed_mutex_lock())
            {
                account_balance += ((host_id + 1) * 10);
                account_balance -= host_id;
                account_balance += ((host_id + 1) * 10);
                account_balance -= ((host_id + 1) * 3);
                printf("account Balance: %u", account_balance);
                distributed_mutex_unlock();
            }
        }
    }
    else
    {
        fprintf(stderr, "Error initializing distributed mutex\n");
        return 1;
    }

    // // If this host is the first host (host_id == 0), send HELLO messages to other hosts
    // if (host_id == 0)
    // {
    //     for (int i = 0; i < MAX_HOSTS; i++)
    //     {
    //         if (host_id != hosts[i].id)
    //         {
    //             char *temp_hostname = (char *)malloc(strlen(hosts[i].hostname) + strlen(".eecs.csuohio.edu") + 1);
    //             if (temp_hostname == NULL)
    //             {
    //                 perror("Memory allocation failed");
    //                 return 1;
    //             }
    //             strcpy(temp_hostname, hosts[i].hostname);
    //             strcat(temp_hostname, ".eecs.csuohio.edu");

    //             msg.cmd = htons(HELLO);
    //             msg.seq = htons(1);
    //             msg.hostid = htonl(host_id);
    //             msg.tiebreak = htonl(getpid());
    //             msg.vtime[0] = htons(0);
    //             msg.vtime[1] = htons(1);
    //             msg.vtime[2] = htons(0);
    //             msg.vtime[3] = htons(0);
    //             msg.vtime[4] = htons(0);

    //             send_udp_message(socket_fd, host_id, "HELLO", temp_hostname, msg);
    //             printf("Sent HELLO message to %s\n", temp_hostname);
    //             free(temp_hostname);
    //         }
    //     }
    // }
    while (1)
        ;
    return 0;
}
