#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "myudp.h"

int main(int argc, char **argv)
{
    int socket_fd, cc;
    struct sockaddr_in dest;
    struct hostent *gethostbyname(), *hostptr;

    struct msg_packet mymsg;

    if (argc != 2)
    {
        printf("correct execution format: send_udp hostname\n");
        exit(0);
    }

    /*
       Set up a datagram (UDP/IP) socket in the Internet domain.
       We will use it as the handle thru which we will send a
       single datagram. Note that, since this no message is ever
       addressed TO this socket, we do not bind an internet address
       to it. It will be assigned a (temporary) address when we send
       a message thru it.
    */

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
    {
        perror("send_udp:socket");
        exit(1);
    }

    /*
       The inet socket address data structure "dest" will be used to
       specify the destination of the datagram. We must fill in the
       blanks.
    */

    bzero((char *)&dest, sizeof(dest)); /* They say you must do this */
    if ((hostptr = gethostbyname(argv[1])) == NULL)
    {
        fprintf(stderr, "send_udp: invalid host name, %s\n", argv[1]);
        exit(1);
    }
    dest.sin_family = AF_INET;
    bcopy(hostptr->h_addr_list[0], (char *)&dest.sin_addr, hostptr->h_length);
    dest.sin_port = htons((unsigned short)0x3333);

    mymsg.cmd = htons(HELLO);
    mymsg.seq = htons(1);
    mymsg.hostid = htonl(1);
    mymsg.tiebreak = htonl(getpid());
    mymsg.vtime[0] = htons(0);
    mymsg.vtime[1] = htons(1);
    mymsg.vtime[2] = htons(0);
    mymsg.vtime[3] = htons(0);
    mymsg.vtime[4] = htons(0);

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

    cc = sendto(socket_fd, &mymsg, sizeof(struct msg_packet), 0, (struct sockaddr *)&dest,
                sizeof(dest));
    if (cc < 0)
    {
        perror("send_udp:sendto");
        exit(1);
    }

    return (0);
}
