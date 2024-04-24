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
#include "../myudp.h"
#include "../udp_procedures.h"

int send_udp_message(int socket_fd, int hostid, const char *message, const char *destination, struct msg_packet mymsg)
{
    struct sockaddr_in dest;
    struct hostent *gethostbyname(), *hostptr;

    /*
       The inet socket address data structure "dest" will be used to
       specify the destination of the datagram. We must fill in the
       blanks.
    */

    bzero((char *)&dest, sizeof(dest)); /* They say you must do this */
    if ((hostptr = gethostbyname(destination)) == NULL)
    {
        // fprintf(stderr, "send_udp: invalid host name, %s\n", destination);
        return -1; // Indicate failure
    }
    dest.sin_family = AF_INET;
    bcopy(hostptr->h_addr_list[0], (char *)&dest.sin_addr, hostptr->h_length);
    dest.sin_port = htons((unsigned short)UDP_PORT);

    // mymsg.seq = htons(1);
    // mymsg.hostid = htonl(hostid);
    // mymsg.tiebreak = htonl(getpid());
    // mymsg.vtime[0] = htons(0);
    // mymsg.vtime[1] = htons(1);
    // mymsg.vtime[2] = htons(0);
    // mymsg.vtime[3] = htons(0);
    // mymsg.vtime[4] = htons(0);

    /*
        mymsg.cmd = HELLO;
        mymsg.seq = 1;
        mymsg.hostid = 1;
        mymsg.tiebreak = (unsigned int) getpid();
        mymsg.vtime[0] = 0;
        mymsg.vtime[1] = 1;
        mymsg.vtime[2] = 0;
        mymsg.vtime[3] = 0;
        mymsg.vtime[4] = 0;
    */

    ssize_t cc = sendto(socket_fd, &mymsg, sizeof(struct msg_packet), 0, (struct sockaddr *)&dest,
                        sizeof(dest));
    if (cc < 0)
    {
        perror("send_udp:sendto");
        return -1; // Indicate failure
    }

    return 0; // Indicate success
}