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

#include "user.h"

// Create a node
struct user *create_user(int status, const char *name, const char *group, struct sockaddr_in addr)
{
  struct user *u = malloc(sizeof(struct user));
  u->user_status = status;

  strncpy(u->user_name, name, 29);
  u->user_name[29] = '\0';
  
  strncpy(u->user_group, group, 29);
  u->user_group[29] = '\0';
  
  u->user_addr = addr;
  u->next = NULL;
  
  return u;
}


// Loop Through the node to find an element, by user_name, return null if not found
struct user *find_user(struct user *head, const char *name)
{
  struct user *curr;
  for (curr = head; curr != NULL; curr = curr->next) 
  {
    if (strcmp(curr->user_name, name) == 0)
    {
      return curr;
    }
  }

  return NULL;
}


// Loop Through the node to find an element, by IP and Port Number, return null if not found
struct user *find_user_ip(struct user *head,  struct sockaddr_in ip_addr)
{
  struct user *curr;
  for (curr = head; curr != NULL; curr = curr->next) 
  {
    if (memcmp(&curr->user_addr.sin_addr, &ip_addr.sin_addr, sizeof(struct in_addr)) == 0 \
    && curr->user_addr.sin_port == ip_addr.sin_port)
    {
      return curr;
    }
  }

  return NULL;
}


// Loop Through the node and remove an element, by user_name
void remove_user(struct user **head, const char *name)
{
  if(head==NULL)
  {
    return;
  }

  struct user *prev = NULL;
  struct user *curr;

  for (curr = *head; curr != NULL; prev = curr, curr = curr->next) 
  {
    if (strcmp(curr->user_name, name) == 0) 
    {
      if (prev == NULL) 
      {
        *head = curr->next;
      } 
      else
      {
        prev->next = curr->next;
      }
      free(curr);
      return;
    }
  }
  return;
}


// Pop User at first position
void insert_user(struct user **head, struct user *u)
{
  u->next = *head;
  *head = u;
}


// Remove entire list, used when udpating clients
void destroy_list(struct user **head)
{
  struct user *curr = *head;

  while (curr != NULL) 
  {
    struct user *next = curr->next;
    free(curr);
    curr = next;
  }
  *head = NULL;
  return;
}


// Print User
char *print_user(struct user *curr)
{
  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(curr->user_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
    
  char *output = malloc(128);
  sprintf(output, "%d %s %s %s %d",curr->user_status, curr->user_name, curr->user_group, addr_str, ntohs(curr->user_addr.sin_port));

  return output;
}


// Print entire list
void print_list(struct user *head)
{
  struct user *curr;
  for (curr = head; curr != NULL; curr = curr->next) 
  {
    printf("%s\n", print_user(curr));
  }
}


/* Used for Debugging in separate files */

/////////////////////////////////////////////////////////////////
// int main()
// {
//   struct user *head = NULL;
//   struct sockaddr_in addr = {0};
//   addr.sin_family = AF_INET;
//   addr.sin_port = htons(1145);
//   addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//   insert_user(&head, create_user(0, "Alice", "Group1", addr));
//   insert_user(&head, create_user(0, "Bob", "Group2", addr));
//   insert_user(&head, create_user(0, "Charlie", "Group1", addr));
//   insert_user(&head, create_user(0, "Daniel", "Group1", addr));
//   insert_user(&head, create_user(0, "Emily", "Group3", addr));
//   insert_user(&head, create_user(0, "Frank", "Group1", addr));
//   insert_user(&head, create_user(0, "Garcia", "Group2", addr));
//   insert_user(&head, create_user(0, "Henery", "Group2", addr));
//   insert_user(&head, create_user(0, "Izumo", "Group2", addr));
//   insert_user(&head, create_user(0, "Julia", "Group2", addr));
//   insert_user(&head, create_user(0, "Kenya", "Group2", addr));
//   insert_user(&head, create_user(0, "Lucy", "Group2", addr));
//   insert_user(&head, create_user(0, "Magdalen", "Group2", addr));
//   insert_user(&head, create_user(0, "Nancy", "Group2", addr));
//   print_list(head);
//   return 0;
// }
////////////////////////////////////////////////////////////////

// Create a gp
struct gp *create_gp(const char *name)
{
  struct gp *u = malloc(sizeof(struct gp));

  strncpy(u->gp_name, name, 29);
  u->gp_name[29] = '\0';
  u->next = NULL;
  
  return u;
}


// Loop Through the node to find an element, by gp_name
struct gp *find_gp(struct gp *head, const char *name)
{
  struct gp *curr;
  for (curr = head; curr != NULL; curr = curr->next) 
  {
    if (strcmp(curr->gp_name, name) == 0)
    {
      return curr;
    }
  }

  return NULL;
}


// Pop at First position
void insert_gp(struct gp **head, struct gp *u)
{
    u->next = *head;
    *head = u;
}


// Loop Through the node and remove an element, by user_name
void remove_gp(struct gp **head, const char *name)
{
  if(head==NULL)
  {
    return;
  }

  struct gp *prev = NULL;
  struct gp *curr;

  for (curr = *head; curr != NULL; prev = curr, curr = curr->next) 
  {
    if (strcmp(curr->gp_name, name) == 0) 
    {
      if (prev == NULL) 
      {
        *head = curr->next;
      } 
      else
      {
        prev->next = curr->next;
      }
      free(curr);
      return;
    }
  }
  return;
}


// Print all current GP
void print_gp(struct gp *head)
{
  printf(">>> [Available Group Chats:]\n");
  fflush(stdout);
  struct gp *curr;
  for (curr = head; curr != NULL; curr = curr->next) 
  {
    printf(">>> %s\n", curr->gp_name);
    fflush(stdout);
  }
}