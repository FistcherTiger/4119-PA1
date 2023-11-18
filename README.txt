# Instructions for compiling the program:

My submission should contain 10 files: README.txt, test.txt, Makefile, ChatApp.c, interface.c, interface.h, udp.c, udp.h, user.c, and user.h.

Put all files in one folder under linux environment and type make. You should see the following:
cc -Wall -c ChatApp.c
cc -Wall -c udp.c
cc -Wall -c interface.c
cc -Wall -c user.c
cc -Wall -o ChatApp ChatApp.o udp.o interface.o user.o -lpthread

Then you type ls and you should see an executable called ChatApp, run ./ChatApp -h for seeing how to start the program.

# Implementation Overview

I put codes for server and client into one program, but implementation-wise the two modes runs their own code. 

For both modes, however, the program is running on two threads: one on main() and one on *network_thread_f(). The main thread deals with user inputs and is responsible for counting the timer for timeout events. The network thread listens on the UDP port binded and process the data based on the type of messages received (more on message section), also it is the network threads that gets the interrupt for timer. For example, say client send a message which requires an ACK from server. The client sets up timer on main thread and send out data, when ACK arrives from server it arrives at network thread which resets the timer on the main thread such that the main thread can proceed. The core logic should be very clear by looking at main() function in ChatApp.c and *network_thread_f() function in udp.c. Note these two infinite loops run concurrently.

# Files

Makefile is used to compile the program. "make clean" remove all the object files and executables, allowing a clean complie. "make remove" removes everything, be cautious.

ChatApp.c is the main function, describing the main thread and deal initial processing of arguments passed in.

interface.c interface.h, this file contains how the program deal with commands/received messages, in the form of 4 functions:
1. process_command(char *command); Describes how to deal with user commands in private mode of client mode
2. process_command_group(char *command); Describes how to deal with user commands in group mode of client mode
3. process_datagram_client(char *datagram, struct sockaddr_in client_addr); Described how to deal with incoming messages into network thread, in client mode
4. process_datagram_server(char *datagram, struct sockaddr_in client_addr); Described how to deal with incoming messages into network thread, in server mode

udp.c udp.h. this file contains how the program sets up UDP ports, how the program sets up network thread, and how the program send messages
user.c user.h this file contains the data structure used to store user and group information, and functions manipulating/querying the data structure

# Datastructure

This program used linked list to store both the user data(Client and Server) and the group data(Server Only), implementation of both linked lists are in user.h and user.c.

Linked list <user> has four components:
User Name: The unique identifier for the user
User Addr: Combined IPv4 address and port number for the user, it is also unique for the user. The uniqueness is enforced by registration process.
User Group: The group this user belongs to.
User Status: 1 for online, 2 for offline
And the pointer for next <user>

Linked list <group> has only one components:
Group Name: Name of the group
And the pointer for next <group>

An improvement for this data structure is proposed below:

Linked list <user> has three components:
User Name: The unique identifier for the user
User Addr: Combined IPv4 address and port number for the user, it is also unique for the user. The uniqueness is enforced by registration process.
User Status: 1 for online, 2 for offline
And the pointer for next <user>

Linked list <group> has two components:
Group Name: Name of the group
<member>: Linked list to store its members
And the pointer for next <group>

Linked list <member> has one components:
User name: The name of the user who belong to this group
And the pointer for next <member>

Initially I tried to store everything in one table and very soon realized it is very difficult to do, then I tried to switch to the improved data structure I proposed and again find out I need to change too many things from my original one-table structure, which will took too long to do. The data structre used in this program is a compromise.

# Messages

All messages sent and received are in the form "<HEADER> <BODY>" where <HEADER> is a three-character code signifies what this messages are intended to do. Where the body contains extra information that the specific header might need. All the headers uded in this program are listed below:

~ Group Headers
GPA ACK for GrouP-related headers, the program will display what is written in the body on the screen
GPC Create GrouP request
GPJ Join GrouP request
GPQ Quit GrouP request
GPM GrouP Message, for both client to server and server to client.

GPR Request GrouP list
GPL GrouP name package, this type of header only exist if number of group>30
GPE end GrouP name package, is the only reply header if number of groups<30, also act as ACK

GPS GrouP Search (Listing Members)
GPB GrouP member package, this type of header only exist if number of members>40
GPD end GrouP member Package, is the only reply header if number of members<40, also act as ACK

~ Non-Group Headers
REG Change Regsitration Status, for both registering and de-registering
MSG Private Message
UPD Update User Linked List
UPT Requesting Update User Linked List, refresh the list for everyone
FIN End Update User Linked List, also act as ACK

# Commands In Non-Group Chat Mode

All inputs in non-group chat mode are treated as <command> + <arg1> + <arg2>, where the first word encountered in input is <command>, second word encountered is <arg1> (separated by space), all other words following is treated as single argument <arg2>. Note that <arg1> and <arg2> may be empty string, if no inputs are found.

Actions by the program is decided by <command>, for those command who requires <arg1> or both args, input verification is done inside the branch for that command.

Available Commands are:

1. debug_mode, swtich on and off between debug modes, all the extra outputs for debugging will only show when debug_mode==1
2. send, format same as required, look up local table to find the IP and status, if inactive stop sending, if active send and wait for ACK
3. list_client, list all the ACTIVE clients based on your local table (i.e. non-active client will not show)
4. dereg, same as required
5. create_group, same as required
6. list_groups, same as required, data has to be sent from server
7. join_group, same as required
8. update_table, request the server to update table for all clients, initially debug mode only

# Commands In Group Chat Mode

All inputs in non-group chat mode are treated as <command> + <arg1>, where the first word encountered in input is <command>, and all other words following is treated as single argument <arg1>. Note that <arg1> may be empty string, if no inputs are found.

Actions by the program is decided by <command>, for those command who requires <arg1>, input verification is done inside the branch for that command.

Available Commands are:

1. debug_mode, swtich on and off between debug modes, all the extra outputs for debugging will only show when debug_mode==1
2. dereg, dereging from group automatically quits the group as well
3. leave_group, same as required
4. list_members, same as required
5. send_group, same as required

# Enforcing Uniquess of User_Addr

When user <A> send registration request to server, server enforces uniqueness of user address(IP+port) by checking:

1. If <A> is in table
   1.1 If the registration IP address is the same as IP address for <A> in table, then continue (duplicate or register success, whatever)
   1.2 If the registration IP address is different from IP address for <A> in table, and the registration IP address is not used by any other users in the table, then continue
   1.3 If the registration IP address is different from IP address for <A> in table, and the registration IP address is used by users other than <A> in table, then stop registration and send duplicate.
2. If <A> is not in table, and the registration IP address is used by users other than <A> in table, then stop registration and send duplicate.

# Known Problems:

1. When user is typing while a message arrives, say the screen looks like below (underscore represents cursor location): 

>>> send cc Tod_

when message arrives, the chat application will display the incoming message and starts another line while maintaining the original typed message without explicitily displaying it.

>>> send cc Tod<bob>: Have you done today's homework?
>>> _

But if you type in: "day is snowing heavily." and press enter, i.e.:

>>> send cc Tod<bob>: Have you done today's homework?
>>> day is snowing heavily._

The system will treat it as:
>>> send cc Today is snowing heavily. 

This is because of the blocking nature of I/O in C, difficulty encountered during attempting to read stdin, and print editable outputs (which alone seems to be an equally big project as the entire XXXXXXXXXXXXXXXXXX). I choose this way to display the message as it is implementation-wise the easiest.


# Assumptions:

1. Any extra arguments when starting up the server/client will be ignored. i.e.:

./ChatApp -s 5000 192.168.1.100

will be regarded as:

./ChatApp -s 5000

2. Similar to 1., while the program is running, any extra arguments in command will be ignored. i.e.:

>>> (some_group_name) leave_group Yeah I finaly left this group hahahaha! 

when pressing enter, program treat this as

>>> (some_group_name) leave_group

since command leave_group do not take any arguments, thus any arguments passed in will be ignored.

3. I disallowed special characters for username, including space.

4. I disallowed special characters for group name, including space, but allowing underscore
