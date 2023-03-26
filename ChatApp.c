#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

/* Custom Headers */
#include "udp.h"
#include "interface.h"
#include "user.h"

#define MAX_BUFFER_SIZE 1024                            // Maximum body size for a UDP message
#define PORT_NUMBER 11451                               // Default Port NUmber

/* Helper Functions */
int process_arguments(int argc, char *argv[]);
void broadcast();
int str_to_portnum(char *str);

struct user *user_head = NULL;                          // User List Head for this program
struct gp *gp_head = NULL;                              // Group List Head for this program
char your_name[32] = "";                                // Store user_name of this program if in clent mode
char your_group[32] = "None";                           // Store user_group of this program if in clent mode
char body_buf[1025] = "";                               // Store body for calling function broadcast

int debug_mode = 0;                                     // Indicate whether to switch to debug(verbose) mode

int broadcast_flags = 0;                                // Flag to decide whether to broadcast GPM messages
int is_server_mode;                                     // Flag to tell the program to initiallize as server or client 
int vm_port, vm_addr;                                   // Temp variable to store address of server
extern struct itimerval timer;                          // Timer for timeout events


int main(int argc, char *argv[])
{
  /* Process Input Argumets and Set Up corresponding UDP ports */
  process_arguments(argc, argv);

  /* Init input buffer */
  char input[200];
  memset(input, 0, sizeof(input));

  /* Goes into corresponding loops */
  if(is_server_mode == 0)
  {

    if(debug_mode == 1){printf("Client Mode\n");}

    for(;;) 
    {
      /* Input Prompt */
      printf(">>> ");
      fflush(stdout);

      if(strcmp(your_group, "None") != 0)
      {
        printf("(%s) ",your_group);
      }
      fflush(stdout);

      fgets(input, 200, stdin);

      /* Process commands */
      char *command_buffer = strtok(input, "\n");           // Remove the line changer which will be present at the end
      char *command = strtok(command_buffer, " ");          // Partition

      if (command != NULL) 
      {
        if(strcmp(your_group, "None") == 0)
        {
          process_command(command);
        }
        else if(strcmp(your_group, "None") != 0) // Yes it's else, just to make condition clear
        {
          process_command_group(command);
        }
      }
    }
  }
  else if(is_server_mode == 1)
  {

    if(debug_mode == 1){printf("Server Mode\n");}

    for(;;)
    {
      if(broadcast_flags == 1)
      {
        if(debug_mode == 1){printf("Broadcast\n");}
        broadcast_flags = 0;
        broadcast();
      }
    }
  }

  return 0;

}


/* Processing Arguments */
int process_arguments(int argc, char *argv[])
{
  if(argc < 2)
  {
    printf("Error: Please specify program type. Type ./ChatApp -h for help\n");
    exit(1);
  }
  else if(strcmp(argv[1], "-h")==0)                                             /* Help Page */
  {
    printf("ChatApp -s <port> Starts the Server Process, default port number is %d\n", PORT_NUMBER);
    printf("ChatApp -c <name> <server-ip> <server-port> <client-port> Starts Client Process\n");
    printf("Note: all arguments must be specified in Client Process\n");
    //printf("All arguments must be specified in Client Process and all port number must >10000\n");
    exit(0);
  }
  else if(strcmp(argv[1], "-s")==0)                                            /* Server Mode */
  {
    is_server_mode=1;
    if(argc==2)                                                                // No input from user: Use default port
    {
      printf("Port number unspecified, using default port %d\n", PORT_NUMBER);
      port_number=PORT_NUMBER;
    }
    else                                                                       // Input from user at argv[2], if input good use input, if input bad use default
    {
      port_number=str_to_portnum(argv[2]);
      if(port_number==-1)
      {
        printf("Invalid port number, using default port %d\n", PORT_NUMBER);
        port_number=PORT_NUMBER;
      }
    }

    initialize_udp_server();                                                   // Initialize UDP 

  }
  else if(strcmp(argv[1], "-c")==0)                                            /* Client Mode */
  {

    is_server_mode=0;
    int ret;
    regex_t regex;
    regcomp(&regex, "[^a-zA-Z0-9 ]", 0);

    if(argc<6)                                                                 // No enough input from user
    {
      printf("Error: Insufficient arguments for starting Client Server, see ./ChatApp -h for help\n");
      exit(1);
    }
    else if((ret = regexec(&regex, argv[2], 0, NULL, 0))==0)                                                            // Username Contains Special Character
    {
      printf("Error: Username contain special characters, only permit a-z A-Z 0-9\n");
      exit(1);
    }
    else if(strlen(argv[2])>20)                                               // Username Exceeds length (20 characters)
    {
      printf("Error: Username is too long, program only allows length up to 20\n");
      exit(1);
    }
    else if(inet_pton(AF_INET, argv[3], &vm_addr) == 0)                     // Invalid server IP                                                  
    {
      printf("Error: Invalid input for Server IP address, see ./ChatApp -h for help\n");
      exit(1);
    }
    else if((vm_port = str_to_portnum(argv[4])) == -1)                         // Invalid Server port number
    {
      printf("Error: Invalid input for Server port number, see ./ChatApp -h for help\n");
      exit(1);
    }
    else if((port_number = str_to_portnum(argv[5])) == -1)                     // Invalid Client port number
    {
      printf("Error: Invalid input for Client port number, see ./ChatApp -h for help\n");
      exit(1);
    }
    else
    {
      initialize_udp_client(argv[2],vm_addr,vm_port,port_number);             // All good, then initialize UDP 
    }

  }
  else                                                                         // All other situations
  {
    printf("Error: Invalid arguments. Type ./ChatApp -h for help\n");
    exit(1);
  }

  return 0;
}


/* Broadcast Group Message to everyone */
void broadcast()
{
  int is_user_head_updated = 0;

  // Iteratively send message to anyone whose group chat name matches, if no response de-reg them
  struct user *temp;
  if((temp = find_user_ip(user_head,client_addr)) != NULL)    // Get whom the message is from
  {
    printf(">>> [Client <%s> sent group message: %s]\n",temp->user_name,body_buf);
    // Prepare the Forwarding Message
    // Note that body is capped to 800 character as input limit

    char msg[1024];
    int if_success;
    sprintf(msg, "GPM Group_Message <%s>: %s", temp->user_name, body_buf);

    if(debug_mode == 1){printf("GPM Group_Message <%s>: %s\n", temp->user_name, body_buf);}

    struct user *curr;
    for (curr = user_head ; curr != NULL ; curr = curr->next) 
    {
      if(strcmp(curr->user_group, temp->user_group) == 0 && strcmp(curr->user_name, temp->user_name) != 0)    // If group name matches send message                     
      {
        if((if_success = send_msg(socket_num,msg,curr->user_addr,5))<0)                                       // If 5 send fails, deregister the client
        {
          if(debug_mode == 1){printf("Deregister occurs for: %s\n",curr->user_name);}
          if(debug_mode == 1){printf("Return value: %d\n ",if_success);}
          
          is_user_head_updated = 1;
          curr->user_status = 2;
          strcpy(curr->user_group, "None");

          printf(">>> [Client <%s> not responsive, removed from group <%s>]\n",curr->user_name,temp->user_group);
        }
      }
    }

    if(is_user_head_updated == 1)       // If any client has been deregistered, broadcast the updated list to all clients
    {
      send_user_list(user_head);
    }
  }
  else
  {
    printf(">>> [GPM Request received from invalid client]\n");
  }
}


/* Convert String to Portnumber, invalid input or out-of range return -1 */
int str_to_portnum(char *str)
{
  int port_temp = atoi(str);

  if (port_temp < 1 || port_temp > 65535)   // Note that invalid str input will return 0 in atoi
  {
    return -1;
  }

  return port_temp;
}