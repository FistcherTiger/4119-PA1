#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "interface.h"
#include "udp.h"
#include "user.h"

int init_flags = 0;               // Flag to make sure Message [Client table updated.] do not appear when client first initializes
int is_gp;                        // Helper flag to make sure [Available group chats:] and [Members in the group <group_name>:] only showed once
char buf1[1024] = "";             // String buffer used to store incoming private message while in group mode
char buf2[1024] = "";             // String buffer used to store second incoming private message

/* process_command and process_command_client are on the main thread*/

/* Responsible for processing command in client mode, non-group chat */
void process_command(char *command) 
{
  char *arg1 = strtok(NULL, " "); // get the first argument

  char arg2[1000] = "";           // get the second argument (if exists) Note that eveything beyond the first argument is the second argument
  char *arg2_temp = strtok(NULL, " ");
  while(arg2_temp != NULL)
  {
    char *temp = " ";
    strcat(arg2,temp);
    strcat(arg2,arg2_temp);
    arg2_temp = strtok(NULL, " ");
  }

  /* Switch Betwwen Commands */
  if (strcmp(command, "debug_mode") == 0)                                              // Debug_mode
  {
    if(debug_mode==1)
    {
      printf("Debug Mode Off\n");
      debug_mode=0;
    }
    else if(debug_mode==0)
    {
      printf("Debug Mode On\n");
      debug_mode=1;
    }
  } 
  else if (strcmp(command, "send") == 0)                                               // Command send: Send Message
  {

    // Check whether this machine is in group chat
    struct user *temp_you = find_user(user_head,your_name);
    struct user *temp_dest = NULL;

    if(temp_you==NULL)
    {
      printf(">>> Fatal Error: Incomplete local table\n");
      exit(1);
    }

    if(strcmp(temp_you->user_group, "None") == 0)                                           // Not in group chat
    {
      if(arg1==NULL || arg2==NULL || strcmp(arg1,"")==0 || strcmp(arg2,"")==0)                                                    
      {
        printf(">>> [Error: Invalid arguments, should be in form: send <name> <message>]\n");
      }
      else if(strlen(arg2)>1000)
      {
        printf(">>> [Error: Message is too long]\n");
      }
      else if(strcmp(arg1, your_name) == 0)                                            
      {
        printf(">>> [Error: Can not send message to yourself]\n");
      }
      else if((temp_dest = find_user(user_head,arg1)) == NULL)   
      {
        printf(">>> [Error: Specified username is not registered]\n");
      }
      else if(temp_dest->user_status==2)
      {
        printf(">>> [%s is offline]\n",temp_dest->user_name);
      }
      else // Send message
      {
        char msg[1024];
        sprintf(msg,"MSG %s",arg2);

        if(send_msg(socket_num,msg,temp_dest->user_addr,5)<0)                           // If 5 send fails, notify server
        {
          printf(">>> [No ACK from %s, message not delivered]\n",arg1);

          char msg_addr_str[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &temp_dest->user_addr.sin_addr, msg_addr_str, INET_ADDRSTRLEN);
          
          char msg[128];
          sprintf(msg, "REG %d %s None %s %d",2,temp_dest->user_name,msg_addr_str,ntohs(temp_dest->user_addr.sin_port));
          
          if(send_msg(socket_num,msg,vendim_addr,5)<0)
          {
            printf(">>> [Server not responding]\n");
            printf(">>> [Exiting]\n");
            fflush(stdout);
            exit(0);
          }

          if(debug_mode == 1){printf("Dereg Complete");}

        }
        else
        {
          printf(">>> [Message received by %s]\n",arg1);
        }
      }
    }
    else                                                                               // In group Chat, should not happen,
    {
      print_prompt();
      printf("[Fatal Error: Inconsistent Local State]\n");
      fflush(stdout);
      exit(1);
    }
  } 
  else if (strcmp(command, "list_client") == 0)                                      // Command list_client: List available chat clients
  {
    printf(">>> [List Active Clients:]\n");
    struct user *curr;
    for (curr = user_head; curr != NULL; curr = curr->next) 
    {
      if(curr->user_status!=2 && strcmp(curr->user_name,your_name) != 0)
      {
        printf(">>> %s\n",curr->user_name);
      }
    }
  } 
  else if (strcmp(command, "dereg") == 0)                             // Command dereg: De-register and quit, implementation uses header REG
  {
    struct user *temp_you;
    if((temp_you = find_user(user_head,your_name)) == NULL)
    {
      printf(">>> Fatal Error: Incomplete local table\n");
      exit(1);
    }

    // Send dereg request
    char msg_addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &temp_you->user_addr.sin_addr, msg_addr_str, INET_ADDRSTRLEN);

    char msg[128];
    sprintf(msg, "REG %d %s None %s %d",2,temp_you->user_name,msg_addr_str,ntohs(temp_you->user_addr.sin_port));

    if(send_msg(socket_num,msg,vendim_addr,5)<0)
    {
      printf(">>> [Server not responding]\n");
      printf(">>> [Exiting]\n");
      fflush(stdout);
      exit(0);
    }
    else
    {
      printf(">>> [You are Offline. Bye.]\n");
      exit(0);
    }
  }
  else if(strcmp(command, "list_groups") == 0)                      // Command list_groups: list available group chats
  {
    // Send Group List Request to server
    char temp[] = "GPR ";
    is_gp=0;

    if(send_msg(socket_num,temp,vendim_addr,5)<0)
    {
      printf(">>> [Server not responding]\n");
      printf(">>> [Exiting]\n");
      fflush(stdout);
      exit(1);
    }

  }
  else if(strcmp(command, "create_group") == 0)                    // Command create_group: Create a new group
  {
    int ret;
    regex_t regex;
    regcomp(&regex, "[^a-zA-Z0-9 _]", 0);
    
    if(arg1 == NULL || strcmp(arg1,"") == 0)
    {
      printf(">>> [No specified group name]\n");
    }
    else if(arg2 != NULL && strcmp(arg2,"") != 0)
    {
      printf(">>> [Error: No space is allowed in group name, underscore is allowed instead]\n");
    }
    else if(strlen(arg1)>=20)
    {
      printf(">>> [Error: Group name is too long.]\n");
    }
    else if((ret = regexec(&regex, arg1, 0, NULL, 0))==0)
    {
      printf(">>> [Error: Group name can not contain special characters except underscore]\n");
    }
    else if(strcmp(arg1, "None") == 0)
    {
      printf(">>> [Error: Group Name <None> is reserved]\n");
    }
    else
    {
      char msg[32] = "GPC";
      strcat(msg," ");
      strcat(msg,arg1);
      if(send_msg(socket_num,msg,vendim_addr,5)<0)
      {
        printf(">>> [Server not responding.]");
        printf(">>> [Exiting]");
        exit(1);
      }
    }

  }
  else if(strcmp(command, "join_group") == 0)                       // Command join_group, screening logic is the same as create_group
  {
    int ret;
    regex_t regex;
    regcomp(&regex, "[^a-zA-Z0-9 _]", 0);

    if(arg1 == NULL || strcmp(arg1,"") == 0)
    {
      printf(">>> [No specified group name]\n");
    }
    else if(strlen(arg1)>20)
    {
      printf(">>> [Error: Group name is too long.]\n");
    }
    else if((ret = regexec(&regex, arg1, 0, NULL, 0))==0)
    {
      printf(">>> [Error: Group name can not contain special characters except underscore]\n");
    }
    else if(strcmp(arg1, "None") == 0)
    {
      printf(">>> [Error: Group Name <None> is reserved]\n");
    }
    else                                                                                    // Sending GPJ is almost same as GPC
    {
      char msg[32] = "GPJ";
      strcat(msg," ");
      strcat(msg,arg1);
      if(send_msg(socket_num,msg,vendim_addr,5)<0)
      {
        printf(">>> [Server not responding.]");
        printf(">>> [Exiting]");
        exit(1);
      }
    }
  }
  else if(strcmp(command, "update_table") == 0) // && debug_mode == 1)                      // Manually call update on table
  {
    char msg[32] = "UPT ";
    if(send_msg(socket_num,msg,vendim_addr,0 )<0)
    {
      printf(">>> [Server not responding.]");
      printf(">>> [Exiting]");
      exit(1);
    }
  }
  else 
  {
    print_prompt();
    printf("[Invalid command]\n");
    fflush(stdout);
  }
}


/* Responsible for processing commands in client-mode, in group chat */
void process_command_group(char *command)
{
  char arg1[1000] = "";           // get the first argument (if exists) Note that dfferent from process_command everything after the command is a single arg
  char *arg1_temp = strtok(NULL, " ");
  while(arg1_temp != NULL)
  {
    char *temp = " ";
    if(strcmp(arg1, "") != 0)
    {
      strcat(arg1,temp);
    }
    strcat(arg1,arg1_temp);
    arg1_temp = strtok(NULL, " ");
  }

  if (strcmp(command, "debug_mode") == 0)                                             // Debug_mode
  {
    if(debug_mode==1)
    {
      printf("Debug Mode Off\n");
      debug_mode=0;
    }
    else if(debug_mode==0)
    {
      printf("Debug Mode On\n");
      debug_mode=1;
    }
  }
  else if (strcmp(command, "dereg") == 0)  // Command dereg: De-register and quit, implementation same as non-group chat mode, note dereg automatically quits group
  {
    struct user *temp_you;
    if((temp_you = find_user(user_head,your_name)) == NULL)
    {
      printf(">>> Fatal Error: Incomplete local table\n");
      exit(1);
    }

    // Send dereg request
    char msg_addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &temp_you->user_addr.sin_addr, msg_addr_str, INET_ADDRSTRLEN);

    char msg[128];
    sprintf(msg, "REG %d %s None %s %d",2,temp_you->user_name,msg_addr_str,ntohs(temp_you->user_addr.sin_port));

    if(send_msg(socket_num,msg,vendim_addr,5)<0)
    {
      printf(">>> [Server not responding]\n");
      printf(">>> [Exiting]\n");
      fflush(stdout);
      exit(0);
    }
    else
    {
      printf(">>> [You are Offline. Bye.]\n");
      exit(0);
    }
  }
  else if(strcmp(command, "leave_group") == 0)                                        // Quit Group Chat
  {
    char msg[] = "GPQ ";

    if(send_msg(socket_num,msg,vendim_addr,5)<0)
    {
      printf(">>> [Server not responding]\n");
      print_prompt();
      printf("[Exiting]\n");
      fflush(stdout);
      exit(0);
    }
  }
  else if(strcmp(command, "list_members") == 0)                                       // Request Listing Member
  {
    char msg[] = "GPS ";
    is_gp=0;

    if(send_msg(socket_num,msg,vendim_addr,5)<0)
    {
      printf(">>> [Server not responding]\n");
      print_prompt();
      printf("[Exiting]\n");
      fflush(stdout);
      exit(0);
    }
  }
  else if(strcmp(command, "send_group") == 0)                                       // Sending message to groups
  {
    if(arg1 == NULL || strcmp(arg1,"") == 0)                                        // Screening
    {
      print_prompt();
      printf("[Message can not be empty]\n");
    }
    else if(strlen(arg1)>800)     // The cap on length for group message is a bit shorter as header added on group msg is expected to be longer
    {
      print_prompt();
      printf("[Message is too long]\n");
    }
    else                                                                            // Send Message
    {
      char msg[1024];
      sprintf(msg, "GPM %s\n", arg1);

      if(send_msg(socket_num,msg,vendim_addr,5)<0)                                   // If 5 send fails, exit
      {
        printf(">>> [Server not responding]\n");
        fflush(stdout);
        print_prompt();
        printf("[Exiting]\n");
        fflush(stdout);
        exit(0);
      }
    }
  }
  else
  {
    print_prompt();
    printf("[Invalid command]\n");
    fflush(stdout);
  } 

}

/* process_datagram functions runs on the network thread*/

/* Processing incoming messages for clients */
void process_datagram_client(char *datagram, struct sockaddr_in client_addr)
{
  char body[1024] = "";           // Get the body of the UDP datagram, if exists
  char *body_temp = strtok(NULL, " ");
  while(body_temp != NULL)
  {
    char *temp = " ";
    if(strcmp(body, "") != 0)
    {
      strcat(body,temp);
    }
    strcat(body,body_temp);
    body_temp = strtok(NULL, " ");
  }

  if(debug_mode == 1)
  {
    printf("Received: %s %s\n",datagram,body);
    fflush(stdout);
  }

  if(strcmp(datagram, "MSG") == 0 && strcmp(body, "") != 0)                             // Header MSG
  {                                                                                     
    struct user *temp_you, *temp; 
    
    if((temp_you = find_user(user_head,your_name)) == NULL)
    {
      printf(">>> Fatal Error: Incomplete local table\n");
      exit(1);
    }

    if(strcmp(your_group , "None") == 0)                                               // Not in Group
    {
      if((temp = find_user_ip(user_head,client_addr)) != NULL)                         // Unless the message comes from a registered user it won't be displayed
      {
        printf("<%s>: %s\n", temp->user_name, body);
        print_prompt();

        // Send ACK
        char *ack_temp = "ACK ";
        send_msg(socket_num,ack_temp,client_addr,0);
      }
    }
    else if((temp = find_user_ip(user_head,client_addr)) != NULL)                     // In group, store as buffer, 
    {
      // Send ACK
      char *ack_temp = "ACK ";
      send_msg(socket_num,ack_temp,client_addr,0);

      char body_temp[1024] = "";
      sprintf(body_temp,"<%s>: %s",temp->user_name,body);

      if(strcmp(buf1, "") == 0)                                                       // If 1 empty store in 1
      {
        strcpy(buf1,body_temp);
      }
      else if(strcmp(buf2, "") == 0)                                                  // Else if 2 empty store in 2
      {
        strcpy(buf2,body_temp);
      }
      else                                                                            // If both not empty replace 1 with 2, then store in 2
      {
        strcpy(buf1,buf2);
        strcpy(buf2,body_temp);
      }
    }

  }
  else if(strcmp(datagram, "GPM") == 0 && strcmp(body, "") != 0)                        // Header GPM, group message from server to client
  {
    // Send ACK to server
    if(debug_mode == 1){printf(">>> SEND ACK\n");}

    char *ack_temp = "ACK ";
    send_msg(socket_num,ack_temp,client_addr,0);

    if(strcmp(your_group , "None") == 0)                                                // Not in Group, this should not happen, but in this case we ignore it
    {
      ;
    }
    else                                                                                // In group, display message
    {
      printf("%s\n",body);
      print_prompt();
      fflush(stdout);
    }
    
  }
  else if(strcmp(datagram, "ACK") == 0)                                                // Header ACK, reset timer
  {
    is_ack = 1;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    if(debug_mode == 1){printf("Reset Timer");}

  }
  else if(strcmp(datagram, "UPD") == 0)                                                // Header UPD update the userlist
  {
    parse_string_to_user(body);
  }
  else if(strcmp(datagram, "FIN") == 0)                                                // Header FIN finish UPD
  {
    if(init_flags==0)
    {
      init_flags=1;
    }
    else
    {
      printf("[Client table updated.]\n");
      print_prompt();
    }
  }
  else if(strcmp(datagram, "DUP") == 0)                                               // Header DUP duplicate register
  {
    printf(">>> Detected duplicate registration, please choose another username or port number\n");
    exit(0);
  }
  else if(strcmp(datagram, "GPE") == 0)                                              // Received Group List, also act as ack
  {
    is_ack = 1;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    if(is_gp == 0)
    {
      is_gp = 1;
      if(strcmp(body,"") == 0)
      {
        printf(">>> [No available group chats]\n");
      }
      else 
      {
        printf(">>> [Available group chats:]\n");
      }
    }

    char *temp = strtok(body, " ");
    while(temp != NULL)
    {
      printf(">>> %s\n",temp);
      fflush(stdout);
      temp=strtok(NULL, " ");
    }
  }
  else if(strcmp(datagram, "GPL") == 0)                                              // Part of group list arrives, reset timer to 500 ms
  {
    timer.it_value.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, NULL);  

    if(is_gp == 0)
    {
      is_gp = 1;
      if(strcmp(body,"") == 0)
      {
        printf(">>> [No available group chats]\n");
      }
      else 
      {
        printf(">>> [Available group chats:]\n");
      }
    }

    char *temp = strtok(body, " ");
    while(temp != NULL)
    {
      printf(">>> %s\n",temp);
      fflush(stdout);
      temp=strtok(NULL, " ");
    }
  }
  else if(strcmp(datagram, "GPD") == 0)                                              // Received Group member List, also act as ack
  {
    is_ack = 1;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    if(is_gp == 0)
    {
      is_gp = 1;
      print_prompt();
      printf("[Members in the group <%s>:]\n",your_group);
    }

    char *temp = strtok(body, " ");
    while(temp != NULL)
    {
      print_prompt();
      printf("%s\n",temp);
      fflush(stdout);
      temp=strtok(NULL, " ");
    }
  }
  else if(strcmp(datagram, "GPB") == 0)                                              // Part of group member list arrives, reset timer to 500 ms
  {
    timer.it_value.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, NULL);  

    if(is_gp == 0)
    {
      is_gp = 1;
      print_prompt();
      printf("[Members in the group <%s>:]\n",your_group);
    }

    char *temp = strtok(body, " ");
    while(temp != NULL)
    {
      print_prompt();
      printf("%s\n",temp);
      fflush(stdout);
      temp=strtok(NULL, " ");
    }
  }
  else if(strcmp(datagram, "GPA") == 0)                                             // Receive ACK for GP-related requests, reset timer and display message
  {
    is_ack = 1;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    // Deep copy body to a buffer msg2
    char *msg2 = malloc(strlen(body) + 1);
    strcpy(msg2, body);

    char temp1[] = "[Entered group";
    char temp2[] = "[Leave";

    if(strncmp(body, temp1, strlen(temp1)) == 0)                                   // If entered successfully set your_group
    {
      char *temp;
      temp = strtok(body, " ");
      temp = strtok(NULL, " ");
      temp = strtok(NULL, " ");
      strncpy(your_group, temp, sizeof(your_group));                               // Copy the group name, i.e. third word, to your_group
      your_group[sizeof(your_group)-1] = '\0';
      while (strtok(NULL, " ") != NULL);                                           // Make sure strtok reached the end to avoid any problems calling strtok afterwards

      /* Print Received Messages */
      printf(">>> %s\n",msg2);
    }
    else if(strncmp(body, temp2, strlen(temp2)) == 0)                              // If leaving group, your_group to none
    {
      char temp[] = "None";                                                        
      strncpy(your_group, temp, sizeof(your_group));                              
      your_group[sizeof(your_group)-1] = '\0';
      
      /* Print Received Messages */
      print_prompt();
      printf("%s\n",msg2);

      if(strcmp(buf1, "") != 0)                                                    // If buffered message, display them and clear buffer
      {
        print_prompt();
        printf("%s\n",buf1);
        fflush(stdout);
        char bf[1023] = "";
        strcpy(buf1,bf);
      }
      if(strcmp(buf2, "") != 0)
      {
        print_prompt();
        printf("%s\n",buf2);
        fflush(stdout);
        char bf[1023] = "";
        strcpy(buf2,bf);
      }
    }
    else                                                                           // All other conditions do nothing
    {
      /* Print Received Messages */
      print_prompt();
      printf("%s\n",msg2);
    }
  }
}


/* Processing All incoming messages for server */
void process_datagram_server(char *datagram, struct sockaddr_in client_addr)
{
  /* Pre-Processing */
  char body[1024] = "";                                                                               // Get the body of the UDP datagram, if exists
  char *body_temp = strtok(NULL, " ");
  while(body_temp != NULL)
  {
    char *temp = " ";
    if(strcmp(body, "") != 0)
    {
      strcat(body,temp);
    }
    strcat(body,body_temp);
    body_temp = strtok(NULL, " ");
  }

  if(debug_mode == 1)
  {
    printf("Received: %s %s\n",datagram,body);
    fflush(stdout);
  }
  
  /* Switching */
  if(strcmp(datagram, "ACK") == 0)                                                                    // Header ACK, reset timer
  {
    if(debug_mode == 1){printf("ISACK\n");}
    is_ack = 1;
    timer.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
      if(debug_mode == 1){printf("Error setting timer\n");}
    }
    else
    {
      if(debug_mode == 1){printf("Reset Properly\n");}
    }

    fflush(stdout);

  }
  else if (strcmp(datagram, "UPT") == 0)                                                             // Cient request to update table
  {
    // Send ACK
    char *ack_temp = "ACK ";
    send_msg(socket_num,ack_temp,client_addr,0);
    
    // Update List to All
    send_user_list(user_head);
    printf(">>> [Updating list to all]\n");

  }
  else if(strcmp(datagram, "REG") == 0 && strcmp(body, "") != 0)                                      // Header REG, registration/deregistration (including returning users)
  {
    // Basically non-iterative version of parse_string_to_user, except IP address is taken care of in special case of registration
    char* status_temp = strtok(body, " ");
    char* name_temp = strtok(NULL, " ");
    char* group_temp = strtok(NULL, " ");
    char* ip_temp = strtok(NULL, " ");
    char* port_temp = strtok(NULL, " ");

    if(strcmp(ip_temp,"0.0.0.0")==0)                                                                  // This should only occur when the server first initialize
    {
      ip_temp = inet_ntoa(client_addr.sin_addr);
    }

    int status_buf, port_buf;
    struct sockaddr_in addr_buf = {0};

    // If all data are good, send ACK and insert it
    if ((status_temp != NULL && (status_buf = atoi(status_temp)) != 0 ) \
    &&  (name_temp != NULL && strlen(name_temp) <= 30) \
    &&  (group_temp != NULL && strlen(group_temp) <= 30) \
    &&  (ip_temp != NULL && inet_pton(AF_INET, ip_temp, &addr_buf.sin_addr.s_addr) != 0) \
    &&  (port_temp != NULL && (port_buf = str_to_portnum(port_temp)) != -1)) 
    {
      // Insert User to the linked list or modify it if exists
      addr_buf.sin_family = AF_INET;
      addr_buf.sin_port = htons(port_buf);

      struct user *temp = find_user(user_head,name_temp);
      struct user *temp_ip = find_user_ip(user_head,addr_buf);

      if(temp != NULL)
      {
        if(temp->user_status == status_buf && temp->user_status == 1)                         // Duplicate Registration
        {
          char *ack_temp = "DUP ";
          send_msg(socket_num,ack_temp,client_addr,0);
        }
        else if(temp_ip == NULL || strcmp(temp_ip->user_name,temp->user_name) == 0) // Returning Users, dereg, or duplicate dereg, overwrite original data
        {                                                 // Note dereg requests are coded s.t they must satisfy strcmp(temp_ip->user_name,temp->user_name) == 0
          remove_user(&user_head, name_temp);
          insert_user(&user_head, create_user(status_buf, name_temp, group_temp, addr_buf));

          // Send ACK
          char *ack_temp = "ACK ";
          send_msg(socket_num,ack_temp,client_addr,0);

          // BroadCast Updated List to All servers
          send_user_list(user_head);
          printf(">>> [Server Table Updated]\n");
        }
        else // if(temp_ip != NULL && strcmp(temp_ip->user_name,temp->user_name) !=0 ) Other user were registered with this IP, send DUP
        {
          char *ack_temp = "DUP ";
          send_msg(socket_num,ack_temp,client_addr,0);
        }
      }
      else if(find_user_ip(user_head,addr_buf) != NULL)     // By not allowing different user using same port & IP combination, we ensure that user_addr is also unique identifier
      {
        char *ack_temp = "DUP ";
        send_msg(socket_num,ack_temp,client_addr,0);
      }
      else                                                                                   // New Registration
      {
        //printf("Insert\n");
        insert_user(&user_head, create_user(1, name_temp, group_temp, addr_buf));            

        // Send ACK
        char *ack_temp = "ACK ";
        send_msg(socket_num,ack_temp,client_addr,0);

        // BroadCast Updated List to All servers
        send_user_list(user_head);
        printf(">>> [Server Table Updated]\n");
      }
    }

    if(debug_mode == 1){print_list(user_head);}

  }
  else if(strcmp(datagram, "GPR") == 0)                                              // Header GPR, group listing request
  {
    struct user *temp;
    if((temp = find_user_ip(user_head,client_addr)) != NULL)                         // Get whom the request is from
    {
      printf(">>> [Client <%s> requested listing groups, current groups:]\n",temp->user_name);

      int count =1;
      struct gp *curr;
      char msg[1000] = "";

      for (curr = gp_head; curr != NULL; curr = curr->next) 
      {
        printf(">>> %s\n", curr->gp_name);                                          // Print on server
        fflush(stdout);

        if(count % 30 == 0)             // If reach multiple of 30 send message,  to try it one can choose a smaller number, say 2
        {
          // Concat the string
          strcat(msg," ");
          strcat(msg,curr->gp_name);
          count++;

          //Apply Header
          char msg_full[1024] = "GPL ";
          strcat(msg_full,msg);

          // Send message
          send_msg(socket_num,msg_full,client_addr,0);

          // Clear Memory
          memset(msg, 0, sizeof(msg));
        }
        else                                                                        // Else concat the string to long enough
        {
          // Concat the string
          strcat(msg," ");
          strcat(msg,curr->gp_name);
          count++;
        }
        
      }

      if(strcmp(msg,"")==0)
      {
        printf(">>> [No groups are found]\n"); // Can do that on client side as well, if GPE and body is empty, display same message, current implementation is simply display nothing
        fflush(stdout);
      }

      //Apply Header
      char msg_ack[1024] = "GPE ";
      strcat(msg_ack,msg);

      // Send all remaining messages, Message itself act as ACK
      send_msg(socket_num,msg_ack,client_addr,0);

    }
    else
    {
      printf(">>> [GPC Request received from invalid client]\n");
    }
  }
  else if (strcmp(datagram, "GPC") == 0)                                         // Find if group name exists in linked list, if not insert
  {
    char msg_ack[64] = "";

    struct user *temp;
    if((temp = find_user_ip(user_head,client_addr)) != NULL)                     // Get whom the request is from, no request from outside
    {
      if(find_gp(gp_head,body)==NULL)
      {
        insert_gp(&gp_head,create_gp(body));
        sprintf(msg_ack,"GPA [Group <%s> created by Server]",body);
        printf(">>> [Client <%s> created group <%s> successfully]\n",temp->user_name,body);
      }
      else
      {
        sprintf(msg_ack,"GPA [Group <%s> already exists.]",body);
        printf(">>> [Client <%s> creating group <%s> failed, group already exists]\n",temp->user_name,body);
      }

      send_msg(socket_num,msg_ack,client_addr,0);

    }
    else
    {
      printf(">>> [GPR Request received from invalid client]\n");
    }
  }
  else if (strcmp(datagram, "GPJ") == 0)// Group_name on server by our design can not be different from what GPJ is requesting, so we only need to consider 1. Group_name is None, 2. Group_name is the same as body in GPJ
  {
    char msg_ack[64] = "";

    struct user *temp;
    if((temp = find_user_ip(user_head,client_addr)) != NULL)                     // Get whom the request is from
    {

      if(find_gp(gp_head,body)==NULL)                                            // Group name does not exist
      {
        sprintf(msg_ack,"GPA [Group %s does not exist]",body);
        printf(">>> [Client <%s> joining group <%s> failed, group does not exist]\n",temp->user_name,body);
      }
      else if(strcmp(temp->user_group,body) == 0)                                // If already joined the group, meaning previous ACK fails, resend ACK
      {
        sprintf(msg_ack,"GPA [Entered group %s successfully]",body);
      }
      else if(strcmp(temp->user_group,"None") == 0)
      {
        strcpy(temp->user_group, body);                                          // Assign group
        sprintf(msg_ack,"GPA [Entered group %s successfully]",body);
        printf(">>> [Client <%s> joined group <%s>]\n",temp->user_name,body);
      }

      // Send ACK
      send_msg(socket_num,msg_ack,client_addr,0);                                                                       
    }
    else
    {
      printf(">>> [GPJ Request received from invalid client]\n");
    }
  }
  else if(strcmp(datagram, "GPQ") == 0)                                         // Receiving this request de-register the user from the group chat
  {
    char msg_ack[64] = "";

    struct user *temp;
    if((temp = find_user_ip(user_head,client_addr)) != NULL)                    // Get whom the request is from
    {
      // If in group -> assign group to None and send ACK, if group is none already -> means ack is not received, so send same ACK in both cases
      char temp_none[] = "None";

      if(strcmp(temp->user_group, temp_none)!= 0) // If group is not none already, change it to none, send ACK regardless of whether this happens
      {
        printf(">>> [Client <%s> left group %s]\n",temp->user_name,temp->user_group);
        sprintf(msg_ack,"GPA [Leave group chat %s]", temp->user_group);
        strcpy(temp->user_group, temp_none);
      }             

      // Send ACK
      send_msg(socket_num,msg_ack,client_addr,0);
    }
    else
    {
      printf(">>> [GPQ Request received from invalid client]\n");
    }
  }
  else if(strcmp(datagram, "GPS") == 0)                                                            // Header GPS, group member listing request
  {
    struct user *temp;
    if((temp = find_user_ip(user_head,client_addr)) != NULL)                                       // Get whom the request is from
    {
      print_prompt();
      printf("[Client %s requested listing members of group %s:]\n", temp->user_name, temp->user_group);
      fflush(stdout);

      int count =1;
      struct user *curr;
      char msg[1000] = "";

      for (curr = user_head; curr != NULL; curr = curr->next) 
      {
        if(strcmp(temp->user_group, curr->user_group) == 0)
        {
          printf(">>> %s\n", curr->user_name);                 // Print on server
          fflush(stdout);

          if(count % 40 == 0)                                   // If reach multiple of 40 send message, to try it one can choose a smaller number, say 2
          {
            // Concat the string
            strcat(msg," ");
            strcat(msg,curr->user_name);
            count++;

            //Apply Header
            char msg_full[1024] = "GPB ";
            strcat(msg_full,msg);

            // Send message
            send_msg(socket_num,msg_full,client_addr,0);

            // Clear Memory
            memset(msg, 0, sizeof(msg));
          }
          else                                                // Else concat the string to long enough
          {
            // Concat the string
            strcat(msg," ");
            strcat(msg,curr->user_name);
            count++;
          }
        }
        
      }

      //Apply Header
      char msg_ack[1024] = "GPD ";
      strcat(msg_ack,msg);

      // Send all remaining messages, Message itself act as ACK
      send_msg(socket_num,msg_ack,client_addr,0);

    }
    else
    {
      printf(">>> [GPS Request received from invalid client]\n");
    }
  }
  else if(strcmp(datagram, "GPM") == 0)                      // Received Group Message from sender
  {
    // Send ACK back right away
    char msg_ack[64] = "GPA [Message received by Server]\n";
    send_msg(socket_num,msg_ack,client_addr,0);

    strcpy(body_buf,body);
    broadcast_flags = 1;  // Set flags to 1 and let actual broadcast to be done on int main(), allowing network thread to continue lisenting for ACKs

    if(debug_mode == 1){printf("String Cpy Fin\n");}

  }
  else                                                        // Ignore all other headers
  {
    ;
  }
}

/* Receiving UPD and convert it to struct_user format and store it */
/* We do not remove entries from table in all operations */
void parse_string_to_user(char *body)
{
  if (strlen(body) == 0) 
  {
    return;
  }

  if(debug_mode == 1){printf("Body: %s\n",body);}

  char* token = strtok(body, " ");
  while (token != NULL) 
  {
    if(debug_mode == 1){printf("Token: %s\n",body);}

    char* status_temp = token;
    char* name_temp = strtok(NULL, " ");
    char* group_temp = strtok(NULL, " ");
    char* ip_temp = strtok(NULL, " ");
    char* port_temp = strtok(NULL, " ");

    int status_buf, port_buf;
    struct sockaddr_in addr_buf = {0};

    // Check validity of all data
    if ((status_temp != NULL && (status_buf = atoi(status_temp)) != 0 ) \
    &&  (name_temp != NULL && strlen(name_temp) <= 30) \
    &&  (group_temp != NULL && strlen(group_temp) <= 30) \
    &&  (ip_temp != NULL && inet_pton(AF_INET, ip_temp, &addr_buf.sin_addr.s_addr) != 0) \
    &&  (port_temp != NULL && (port_buf = str_to_portnum(port_temp)) != -1)) 
    {
      // Insert User to the linked list or modify it if exists
      addr_buf.sin_family = AF_INET;
      addr_buf.sin_port = htons(port_buf);

      struct user *temp = find_user(user_head,name_temp);

      if(debug_mode == 1){printf("Name_Temp %s\n",name_temp);}

      if(temp==NULL)
      {
        if(debug_mode == 1){printf("Insert\n");}
        insert_user(&user_head, create_user(status_buf, name_temp, group_temp, addr_buf));
      }
      else
      {
        if(debug_mode == 1){printf("Change\n");}
        remove_user(&user_head, name_temp);
        insert_user(&user_head, create_user(status_buf, name_temp, group_temp, addr_buf));
      }

    }

    // Move on to the next user
    token = strtok(NULL, " ");
  }

  if(debug_mode == 1){print_list(user_head);}

}

/* Broadcast user_list on server to all active clients */
int send_user_list(struct user *user_head)
{
  struct user *curr;
  char msg_temp[1024] = "UPD ";
  int count = 1;
  for (curr = user_head ; curr != NULL ; curr = curr->next) 
  {
    if(count % 8 == 0)                                   // If reach multiple of 8 send message
    {
      // Concat the string
      strcat(msg_temp," ");
      strcat(msg_temp,print_user(curr));
      count++;

      // Send message
      for (struct user *cur2 = user_head; cur2 != NULL; cur2 = cur2->next) 
      {
        send_msg(socket_num,msg_temp,cur2->user_addr,0);
      }
      // Clear Memory
      memset(msg_temp, 0, sizeof(msg_temp));
      strcpy(msg_temp, "UPD ");

    }
    else                                                // Else concat the string to long enough
    {
      // Concat the string
      strcat(msg_temp," ");
      strcat(msg_temp,print_user(curr));
      count++;
    }
  }

  // Send all remaining message if there is one
  if(strcmp(msg_temp,"UPD ")!=0)
  {
    for (struct user *cur2 = user_head; cur2 != NULL; cur2 = cur2->next) 
    {
      send_msg(socket_num,msg_temp,cur2->user_addr,0);
    }
  }

  // Clear Memory
  memset(msg_temp, 0, sizeof(msg_temp));
  usleep(500000);

  // Tell All that update is done
  struct user *cur3;
  for (cur3 = user_head; cur3 != NULL; cur3 = cur3->next) 
  {
    if(cur3->user_status!=2)
    {
      send_msg(socket_num,"FIN",cur3->user_addr,0);
    }
  }

  return 0;
}


/* Distinguish between non-group-chat mode and group-chat mode and set prompt accordingly */
void print_prompt()
{
  printf(">>> ");
  if(strcmp(your_group, "None") != 0)
  {
    printf("(%s) ",your_group);
  }
  fflush(stdout);
}