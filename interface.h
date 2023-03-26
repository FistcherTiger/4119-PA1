extern int debug_mode;

char *command;
char *command_buffer;
char *datagram;
char *datagram_buffer;

extern int broadcast_flags;
extern char your_name[32];
extern char your_group[32];
extern char body_buf[1025];

extern struct user *user_head;
extern struct gp *gp_head;
extern struct itimerval timer;

extern int str_to_portnum(char *str);

/* Main Thread Function */
void process_command(char *command);
void process_command_group(char *command);

/* Network Thread Function */
void process_datagram_client(char *datagram, struct sockaddr_in client_addr);
void process_datagram_server(char *datagram, struct sockaddr_in client_addr);

/* Helping functions */
void print_prompt();
void parse_string_to_user(char *body);
int send_user_list(struct user *user_head);