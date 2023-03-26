#define MAX_BUFFER_SIZE 1024
#define PORT_NUMBER 11451

extern int debug_mode;                 // Debug_mode

// Linked List Implementation of Users
struct user
{
    int user_status;                   // 1 login 2 logout
    char user_name[30];
    char user_group[30];
    struct sockaddr_in user_addr;
    struct user *next;
};

struct user *create_user(int status, const char *name, const char *group, struct sockaddr_in addr);
struct user *find_user(struct user *head, const char *name);
struct user *find_user_ip(struct user *head,  struct sockaddr_in ip_addr);                           // Note that user IP will also be an unique identifier since we enforced it in REG requests
void remove_user(struct user **head, const char *name);
void insert_user(struct user **head, struct user *u);
void destroy_list(struct user **head);                                                               // Not used in current implementation
char *print_user(struct user *u);
void print_list(struct user *head);

// Linked List Implementation of Group Names
struct gp
{
    char gp_name[30];
    struct gp *next;
};

struct gp *create_gp(const char *name);
struct gp *find_gp(struct gp *head, const char *name);
void remove_gp(struct gp **head, const char *name);                                                  // Not used in current implemenation
void insert_gp(struct gp **head, struct gp *u);
void print_gp(struct gp *head);
