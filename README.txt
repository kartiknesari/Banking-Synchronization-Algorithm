Group 17:
   Team members and contribution:
      Nithin Sai Bogapathi - 2879115 - 50%
      Kartik Manish Nesari - 2860740 - 50%

Design Detail:

1. Overall
   The project is split in 2 parts,
   The UDP networking platform (25 points) files are stored in the udp directory
   The Distributed Mutex Primitives (60 points) files are stored in distributed_api directory
   Each part features a separate makefile, however, the commands for compiling the executables remains the Sample

   1.1 Makefile commands
      make all:      compile executables and binaries for recv_udp and send_udp
      make recv_udp: compile executables and binaries for recv_udp
      make send_udp: compile executables and binaries for send_udp
      make clean:    remove all executables and binaries 


2. UDP Networking Platform
   Operates sending and receiving of udp packets using udp sockets.
   2.1> Files included.
      a. recv_udp.c: Contains the implementation of sending and receiving packets using udp_networking protocol.
      b. myudp.h: Header file defining message packet structure and constants for message types.
      c. udp_procedures.h: Header file defining function prototypes for UDP procedures.
      d. utilities.h: Header file containing utility functions for reading hosts information from a file.
      e. process.hosts: File containing host information.
      f. Makefile: File for compiling the project.
   
   2.2> Program execution procedure
      - Program start at main function and creates sockets
      - Sockets are bind to the address and port
      - Client connection is created to send and receive udp datagrams
      - Should host id == 0, broadcast HELLO message to others
      - if hostID != 0, receive message and do the following,
         - if command is HELLO, instantly send back a HELLO_REPLY message
      - after sending a message, set a timer of t seconds
      - broadcast a HELLO message after t seconds 

   2.3> Execution steps
      - compile the code using "make all" 
      - execute the program using "./recv_udp"
      - Note: Host 0 should be the last to execute the program

   2.4> Sample Output 
      Given below is the sample output from the first 3 hosts
      Host 0,
         kanesari@turing36:~/CIS620/group17_p3/udp$ ./recv_udp
         message received :: sender ID=1: content=HELLO_REPLY
         message received :: sender ID=4: content=HELLO_REPLY
         message received :: sender ID=2: content=HELLO_REPLY
         message received :: sender ID=3: content=HELLO_REPLY
         message received :: sender ID=1: content=HELLO
         message received :: sender ID=2: content=HELLO
         message received :: sender ID=1: content=HELLO
         message received :: sender ID=2: content=HELLO
         message received :: sender ID=1: content=HELLO
      Host 1,
         kanesari@turing37:~/CIS620/group17_p3/udp$ ./recv_udp
         message received :: sender ID=0: content=HELLO
         message received :: sender ID=0: content=HELLO_REPLY
         message received :: sender ID=2: content=HELLO_REPLY
         message received :: sender ID=4: content=HELLO_REPLY
         message received :: sender ID=3: content=HELLO_REPLY
         message received :: sender ID=2: content=HELLO
         message received :: sender ID=2: content=HELLO_REPLY
         message received :: sender ID=3: content=HELLO_REPLY
         message received :: sender ID=0: content=HELLO_REPLY
      Host 2,
         kanesari@turing38:~/CIS620/group17_p3/udp$ ./recv_udp
         message received :: sender ID=0: content=HELLO
         message received :: sender ID=1: content=HELLO
         message received :: sender ID=4: content=HELLO_REPLY
         message received :: sender ID=1: content=HELLO_REPLY
         message received :: sender ID=3: content=HELLO_REPLY
         message received :: sender ID=0: content=HELLO_REPLY
         message received :: sender ID=1: content=HELLO
         message received :: sender ID=1: content=HELLO_REPLY
         message received :: sender ID=3: content=HELLO_REPLY

   2.5> Issues
      No issues occured in this part



3. Distributed Mutual Exclusion API
   Provides distributed mutex APIs as a middleware for a sample banking program

   3.1> Files Included:
      a. recv_udp.c: Contains the implementation of the receiver side of the distributed mutual exclusion protocol.
      b. myudp.h: Header file defining message packet structure and constants for message types.
      c. udp_procedures.h: Header file defining function prototypes for UDP procedures.
      d. utilities.h: Header file containing utility functions for reading hosts information from a file.
      e. process.hosts: File containing host information.
      f. Makefile: File for compiling the project.

   3.2> Description:

      a. Header Files: Includes necessary header files for socket programming, threading, and other utilities.
      b. Global Variables: Declares global variables including socket file descriptor, host information, vector clock, deferred reply flags, and others.
      b. Utility Functions: Defines utility functions like printsin for printing socket information, broadcast_request for broadcasting request messages, and sig_alrm for handling alarm signals.
      c. Message Listener Thread: Implements a thread function message_listener to listen for incoming messages on the socket. Upon receiving a message, it processes it based on the message type.
      d. Initialization Function: Implements distributed_mutex_init for initializing the distributed mutex. It creates a socket, binds it to a port, and spawns the message listener thread.
      e. Locking Function: Implements distributed_mutex_lock for acquiring the distributed mutex lock. It broadcasts a REQUEST message to other processes and waits for REPLY messages before entering the critical section.
      f. Unlocking Function: Implements distributed_mutex_unlock for releasing the distributed mutex lock. It sends deferred REPLY messages, updates the account balance if necessary, and broadcasts an UPDATE message.
      g. Main Function: Reads host information from a file, initializes the distributed mutex, and starts a loop for simulating account balance transactions under the protection of the mutex.


   3.3> Execution Sequence

      a. Main Function:
         - Reads host information from the `process.hosts` file and obtains the host ID.
         - Initializes the distributed mutex by calling `distributed_mutex_init()`.

      b. Initialization (`distributed_mutex_init`):
         - Creates a UDP socket and binds it to a port for communication.
         - Spawns a thread (`message_listener`) to listen for incoming messages.
         - Initializes global variables including account balance, message replies count, vector clock, and flags.

      c. Locking (`distributed_mutex_lock`):
         - Broadcasts a timestamped REQUEST message to all other processes.
         - Waits for REPLY messages from all other processes before entering the critical section.
         - Should the reply_counter < MAX_HOSTS - 1, block the thread (using conditional wait)
         - If thread is blocked and reply_counter = 4, unblock the thread 

      d. Message Listener (`message_listener`):
         - Listens for incoming messages on the socket.
         - Processes received messages based on their type (REQUEST, REPLY, UPDATE, HELLO).
         - On REQUEST, follow the given algorithm to compare vector timestamps 
         - On REPLY, update reply counter
         - On UPDATE, update account_balance variable and print its value

      e. Unlocking (`distributed_mutex_unlock`):
         - Sends deferred REPLY messages to processes that were waiting.
         - Broadcasts an UPDATE message to inform other processes about changes in the account balance.
         - Releases the critical section.

      f. Account Balance Transactions:
         - Within the main loop, simulates account balance transactions while holding the distributed mutex lock.
         - Depending on the host ID, adjusts the account balance by depositing or withdrawing funds.

   3.4> Execution Instructions
      a. Login to each host and keep it active.
      b. Compile the recv_udp.c using "make all".
      c. Run this command in every server that is open: "./recv_udp". Make sure to change the host id accordingly.
      d. Type (Yes|yes|Y|y) to start the simulation

   3.5> Sample Output 
      Given below is the first line and the last 5 lines of each host's terminals. 
      This is done to shorten the overall output and yet show the synchronization being achieved  
      
      Host 1
         Do you want to start the experiment? (Yes | No) : y
         Account Balance: 2880
         Account Balance: 2885
         Account Balance: 2890
         Account Balance: 2895
         Account Balance: 2900

      Host 2
         Do you want to start the experiment? (Yes | No) : Account Balance: 200
         Account Balance: 2880
         Account Balance: 2885
         Account Balance: 2890
         Account Balance: 2895
         Account Balance: 2900
      Host 3
         Do you want to start the experiment? (Yes | No) : Account Balance: 200
         Account Balance: 2880
         Account Balance: 2885
         Account Balance: 2890
         Account Balance: 2895
         Account Balance: 2900
      Host 4
         Do you want to start the experiment? (Yes | No) : Account Balance: 200
         Account Balance: 2880
         Account Balance: 2885
         Account Balance: 2890
         Account Balance: 2895
         Account Balance: 2900
      Host 5
         Do you want to start the experiment? (Yes | No) : Account Balance: 200
         Account Balance: 2880
         Account Balance: 2885
         Account Balance: 2890
         y                          <- manual synchronization
         Account Balance: 2900
   3.6> Issues
      No issues faced in programming 

