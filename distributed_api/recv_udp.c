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

pthread_cond_t replies_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t replies_mutex = PTHREAD_MUTEX_INITIALIZER;

void printsin(struct sockaddr_in *sin, char *m1, char *m2)
{

    printf("%s %s:\n", m1, m2);
    printf("  family %d, addr %s, port %d\n", sin->sin_family,
           inet_ntoa(sin->sin_addr), ntohs((unsigned short)(sin->sin_port)));
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
    printf("\n");
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
            if (ntohs(received_msg.cmd) == REPLY)
            {
                // printf("Received REPLY message from hostID=%d\n", ntohl(received_msg.hostid));
                pthread_mutex_lock(&replies_mutex);
                msg_replies++;
                pthread_cond_signal(&replies_cv);
                pthread_mutex_unlock(&replies_mutex);
            }
            else if (ntohs(received_msg.cmd) == REQUEST)
            {
                // printf("Received REQUEST message from hostID=%d\n", ntohl(received_msg.hostid));
                int requesting_host_id = ntohl(received_msg.hostid);
                int requesting_pid = ntohl(received_msg.tiebreak);
                int requesting_timestamp = ntohs(received_msg.vtime[requesting_host_id]);

                bool can_send_reply = false;

                // check if reply can be sent
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
                        }
                        else
                            ;
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
                        if (vector_clock[i] < ntohs(received_msg.vtime[i]))
                            vector_clock[i] = ntohs(received_msg.vtime[i]);
                    }
                    for (int i = 0; i < MAX_HOSTS; i++)
                        reply.vtime[i] = htons(vector_clock[i]);
                    // msg.account_balance = reply.account_balance;

                    sendto(socket_fd, &reply, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, sizeof(from));
                }
                else
                {
                    // Defer the reply and set RD[i] = 1
                    // printf("Deferring the reply\n");
                    RD[requesting_host_id] = 1;
                }
            }
            else if (ntohs(received_msg.cmd) == UPDATE)
            {
                account_balance = ntohl(received_msg.account_balance);
                printf("Account Balance: %d\n", account_balance);
            }
            else if (ntohs(received_msg.cmd) == HELLO)
            {
                struct msg_packet reply;
                reply.cmd = htons(HELLO_REPLY);
                reply.seq = htons(1);
                reply.hostid = htonl(host_id);
                reply.tiebreak = htonl(getpid());
                sendto(socket_fd, &reply, sizeof(struct msg_packet), 0, (struct sockaddr *)&from, sizeof(from));
            }
            else
                ;
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
        // printsin(&s_in, "RECV_UDP", "Local socket is:");
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
    // Step 1: broadcast a timestamped message
    msg.cmd = htons(REQUEST);
    msg.seq = htons(1);
    msg.hostid = htonl(host_id);
    msg.tiebreak = htonl(getpid());
    for (int i = 0; i < MAX_HOSTS; i++)
        msg.vtime[i] = htons(vector_clock[i]);
    msg.vtime[host_id] = htons(vector_clock[host_id] + 1);
    msg.account_balance = htonl(account_balance);

    request_flag = true;

    broadcast_request(socket_fd, hosts, host_id, msg);

    // Step 2: Wait for REPLY messages from all other processes
    pthread_mutex_lock(&replies_mutex);
    while (msg_replies < MAX_HOSTS - 1)
        pthread_cond_wait(&replies_cv, &replies_mutex);
    pthread_mutex_unlock(&replies_mutex);

    // printf("Entering critical section\n");
    enter_critical = true;
    request_flag = false;
    return 0;
}

int distributed_mutex_unlock()
{
    // Iterate through the RD array and send deferred REPLY messages
    struct msg_packet reply;
    vector_clock[host_id] += 1;
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
            // printf("Sent deferred REPLY message to hostID=%d\n", j);

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
    // printf("Out of Critical Section\n");
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
    // printf("host id: %d\n", host_id);

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
            sleep(1);
            if (!distributed_mutex_lock())
            {
                switch (host_id)
                {
                case 0:
                    account_balance += 100;
                    break;
                case 1:
                    account_balance += 50;
                    break;
                case 2:
                    account_balance -= 10;
                    break;
                case 3:
                    account_balance -= 5;
                    break;
                case 4:
                    account_balance += 5;
                    break;
                default:
                    account_balance += 0;
                }
                // printf("account Balance: %d\n", account_balance);
                distributed_mutex_unlock();
            }
        }
    }
    else
    {
        fprintf(stderr, "Error initializing distributed mutex\n");
        return 1;
    }
    printf("Account Balance: %d\n", account_balance);
    while (1)
        ;
    return 0;
}
