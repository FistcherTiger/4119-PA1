#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#include "udp.h"
#include "interface.h"
#include "user.h"

int timer_expired = 0;
int is_ack = 0;
struct itimerval timer;
pthread_t network_thread;


void initialize_udp_server()
{
  // Create Socket
  if ((socket_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    printf("Error: Failed to Create Socket");
    exit(1);
  }

  // Init server and client and vending machine address
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));
  memset(&vendim_addr, 0, sizeof(vendim_addr));

  // Filling server information
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;             
  server_addr.sin_port = htons(port_number);
  
  // Bind the socket with the server address
  if (bind(socket_num, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Error: Failed to Bind Port");
    exit(1);
  }

  // Start Network Thread
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  printf(">>> [Initialization Successfull, Hearing on Port %d]\n", port_number);

}


void initialize_udp_client(char *usr_nm,int vm_addr,int vm_port,int port_number)
{
  // Create Socket
  if ((socket_num = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
  {
    printf("Error: Failed to Create Socket\n");
    exit(1);
  }

  // Init server and client and vending machine address
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));
  memset(&vendim_addr, 0, sizeof(server_addr));

  // Filling server and VM information
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;             
  server_addr.sin_port = htons(port_number);

  vendim_addr.sin_family = AF_INET;
  vendim_addr.sin_addr.s_addr = (uint32_t)vm_addr;             
  vendim_addr.sin_port = htons(vm_port);
  
  // Bind the socket with the server address
  if (bind(socket_num, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Error: Failed to Bind Port\n");
    exit(1);
  }

  // Start Network Thread
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  //Send Registration to Server, tried to get sender IP address locally but always get 0.0.0.0, so set that in the Server side
  /* Affected codes */
  struct sockaddr_in temp_addr;
  socklen_t temp_len = sizeof(temp_addr);
  getsockname(socket_num, (struct sockaddr *)&temp_addr, &temp_len);

  char msg_addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &temp_addr.sin_addr, msg_addr_str, INET_ADDRSTRLEN);

  // msg_addr_str is 0.0.0.0
  char msg[128];
  sprintf(msg, "REG %d %s None %s %d",1,usr_nm,msg_addr_str,ntohs(server_addr.sin_port));
  /* End of affected codes */

  if(send_msg(socket_num,msg,vendim_addr,1)<0)
  {
    printf("Error: Registration failed, please check server IP address or port number\n");
    exit(1);
  }
  
  // Set the machine's name after registration succeed
  strcat(your_name,usr_nm);
  printf(">>> [Welcome %s, you are registered.]\n",your_name);

}


/* Send Message Using UDP Sockets, return -1 if all send fails, return -2 if timeout, ACK is triggered externally */
int send_msg(int socket_num, char *buffer, struct sockaddr_in destaddr, int use_timer)
{
  if(debug_mode == 1){printf("send_msg starts, msg: %s, timer: %d\n",buffer,use_timer);}

  int rtn = sendto(socket_num, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&destaddr, sizeof(destaddr));

  // Set the timer for 500 milliseconds & Start the timer, use_timer signifies how many tries will be attempted before timeout
  while(use_timer > 0)
  {
    if(debug_mode == 1){printf("Use_timer: %d Time_expired %d\n",use_timer,timer_expired);}

    signal(SIGALRM, timer_handler);
    timer.it_value.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, NULL);                               
    usleep(500001);                                                     // Wait for 500ms

    if(timer_expired == use_timer)                                        // If number to attempts reaches use_timer, timeout
    {
      if(debug_mode == 1){printf("Send fail\n");}
      
      timer_expired = 0;
      return -2;
    }
    else if(is_ack==1)                                                  // If ACK is called by outside events, stop sending
    {
      if(debug_mode == 1){printf("Send Success\n");}

      is_ack=0;
      return rtn;
    }
    else                                                                // If no ACK while the number is still not maximum, attempt resend
    {
      rtn = sendto(socket_num, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&destaddr, sizeof(destaddr));
    }
  }

  return rtn;

}


/* Network Thread For Processing Incoming Messages */
void *network_thread_f(void *ignored)
{
  /* Initialize */
  int n;
  char buffer[MAX_BUFFER_SIZE];
  unsigned int x = sizeof(client_addr);
  unsigned int *client_addr_len = &x;
  memset(buffer, 0, sizeof(buffer));
  
  /* Receive data & display */
  while( (n = recvfrom(socket_num, (char *)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addr_len)) > 0 )
  {

    /* Display Incoming Msg */
    buffer[n] = '\0';

    char *datagram_buffer = strtok(buffer, "\n");           // Remove the line changer which will be present at the end
    char *datagram = strtok(datagram_buffer, " ");          // Partition

    if(is_server_mode == 0)
    {
      process_datagram_client(datagram, client_addr);
    }
    else if(is_server_mode == 1)
    {
      process_datagram_server(datagram, client_addr);
    }

  }

  return NULL;

}


/* Timeout Event Handler */
void timer_handler()
{
  timer_expired++;
  if(debug_mode == 1){printf("Handler Called, Timer: %d\n",timer_expired);}
}
