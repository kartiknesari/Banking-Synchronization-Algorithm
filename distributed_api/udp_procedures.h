#ifndef UDP_PROCEDURES
#define UDP_PROCEDURES

#define UDP_PORT 0x5273;
int send_udp_message(int socket_fd, int hostid, const char *message, const char *destination);

#endif