CC=gcc
CFLAGS=-g -Wall

all: recv_udp send_udp 

recv_udp: recv_udp.o myudp.h
	$(CC) -o recv_udp recv_udp.o ./utilities/*.c

send_udp: send_udp.o myudp.h
	$(CC) -o send_udp send_udp.o

clean:
	rm -f *.o recv_udp send_udp 
