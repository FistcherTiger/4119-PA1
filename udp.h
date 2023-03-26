#define MAX_BUFFER_SIZE 1024
#define PORT_NUMBER 11451

extern int debug_mode;                // Debug_mode

int port_number;                      // Port Number Used by This Program, Regardless of Server or Client
int socket_num;                       // Socket Used by This Program, Regardless of Server or Client

struct sockaddr_in vendim_addr;       // Address and port number of I called "Vending Machine", aka server (Use this term as server_addr has different meaning in client mode)
struct sockaddr_in server_addr;       // Socket address of the machine the program is running on, not the actual -s Server, which is specifid as vendim_......
struct sockaddr_in client_addr;       // Socket addresses of all incoming messages, not specific to any client

extern int is_ack;
extern int is_server_mode;
extern char your_name[32];

extern struct user *user_head;
extern struct gp *gp_head;
extern struct itimerval timer;

void initialize_udp_server();         // UDP init when program in -s
void initialize_udp_client();         // UDP init when program in -c

int send_msg();                       // Sending message using UDP

void *network_thread_f(void *);       // Thread for hearing
void timer_handler();                 // Event handler for timeout
