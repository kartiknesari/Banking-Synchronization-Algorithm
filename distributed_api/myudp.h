#define HELLO 1
#define HELLO_REPLY 2
#define REQUEST 10
#define REPLY 11
#define UPDATE 12

#define UDP_PORT 0x5835

struct msg_packet
{
  unsigned short cmd;
  unsigned short seq;
  unsigned int hostid;
  unsigned int tiebreak;
  unsigned short vtime[5];
  unsigned int account_balance;
};
