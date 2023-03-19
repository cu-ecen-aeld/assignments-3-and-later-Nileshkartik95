/*****************************************************************************
​* Copyright​ ​(C)​ ​2023 ​by​ ​Nileshkartik Ashokkumar
​*
​* ​​Redistribution,​ ​modification​ ​or​ ​use​ ​of​ ​this​ ​software​ ​in​ ​source​ ​or​ ​binary
​* ​​forms​ ​is​ ​permitted​ ​as​ ​long​ ​as​ ​the​ ​files​ ​maintain​ ​this​ ​copyright.​ ​Users​ ​are
​​* ​permitted​ ​to​ ​modify​ ​this​ ​and​ ​use​ ​it​ ​to​ ​learn​ ​about​ ​the​ ​field​ ​of​ ​embedded
​* software.​ Nileshkartik Ashokkumar ​and​ ​the​ ​University​ ​of​ ​Colorado​ ​are​ ​not​ ​liable​ ​for
​​* ​any​ ​misuse​ ​of​ ​this​ ​material.
​*
*****************************************************************************
​​*​ ​@file​ ​aesdsocket.c
​​*​ ​@brief ​ functionality of the socket client communication
​​*
​​*​ ​@author​ ​Nileshkartik Ashokkumar
​​*​ ​@date​ ​Feb​ ​23​ ​2023
​*​ ​@version​ ​1.0
​*
*/

/************************************include files***************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/queue.h>
#include <pthread.h>

#define BACKLOG (3)
#define MAXDATASIZE (1024)
#define TIMESTAMP_SIZE (128)
#define AELD_CDEV (1)



/***********************************Global Variable***************************/
SLIST_HEAD(client_list_head_t, client_node_s);
struct client_list_head_t client_list_head;

int fd_wr_file;
int fd_soc_server;
bool st_kill_thread = false;
pthread_mutex_t lock;

struct client_node_s
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    pthread_t thread_id;
    char* malloc_wr_buffer;
    SLIST_ENTRY(client_node_s) entries;
};

#ifdef AELD_CDEV
#define SOCK_FILE_WRITE ("/dev/aesdchar")
#else
#define SOCK_FILE_WRITE ("/var/tmp/aesdsocketdata")
#endif

/*****************************Function declaration****************************/
void *thread_new_connection(void *client_data);
int read_datasocket_strlocalbuf(int fd_soc_client, char **malloc_wr_buffer, int *malloc_buffer_len);
int file_read(int fd_wr_file, char **malloc_wr_buffer, int *malloc_buffer_len);
void cleanup_on_exit();
void sig_handler();
#ifndef AELD_CDEV
void * log_timestamp_write();
#endif

/*****************************Function definition******************************/
int main(int argc, char **argv)
{
    int fd_soc_client;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in ip4addr;
    socklen_t addr_size;
    int opt_val = 1;
    int ret_status = 0;
    int deamon_enabled = 0;

    openlog(NULL, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);									/*open connection for sys logging, ident is NULL to use this Program for the user level messages*/
    
    if((signal(SIGINT, sig_handler) == SIG_ERR) || (signal(SIGTERM, sig_handler) == SIG_ERR))
    {
        syslog(LOG_ERR," Signal handler error");
        cleanup_on_exit();
        exit(EXIT_FAILURE);
    }

    /* Check for daemon enabled*/
    if(argc == 2)
    {
        if(strcmp(argv[1],"-d") == 0)
        {
            deamon_enabled = 1;
            syslog(LOG_INFO,"Running this process as Daemon");
        }
        else
        {
            syslog(LOG_ERR,"Invalid 2nd argument for aesdsocket");
            return (EXIT_FAILURE);
        }
    }
    else if(argc > 2)
    {
        syslog(LOG_ERR,"Invalid Number of arguments for aesdsocket");
        return (EXIT_FAILURE);
    }

    /*if the run as daemon argument is present run daemon*/
    if(deamon_enabled == 1)
    {
        if(daemon(0,0) == -1)
        {
            syslog(LOG_ERR,"Daemon fork failure");
            cleanup_on_exit();
            return (EXIT_FAILURE);
        }
    }

    /* open file for Socket data*/
    fd_wr_file = open(SOCK_FILE_WRITE, O_CREAT | O_RDWR | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd_wr_file == -1)
    {
        perror("file open failed\n");
        syslog(LOG_ERR,"file open failed");
        cleanup_on_exit();
        return (EXIT_FAILURE);
    }

    /*create an endpoint for communication */
    int fd_soc_server = socket(AF_INET,SOCK_STREAM,0);
    if(fd_soc_server == -1)
    {
        perror("socket creation failed");
        syslog(LOG_ERR,"socket creation failed");
        cleanup_on_exit();
        return (EXIT_FAILURE);
    }

    // Set socket options for reusing address and port
    if (setsockopt(fd_soc_server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt_val, sizeof(int)))
    {
        printf("Error: %s : Failed to set socket options\n", strerror(errno));
        syslog(LOG_ERR, "Error: %s : Failed to set socket options\n", strerror(errno));
        cleanup_on_exit();
        return -1;
    }

    /*binding the Socket port*/
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if(getaddrinfo(NULL, "9000", &hints, &res)!= 0)
    {
        perror("socket get addr info failed");
        syslog(LOG_ERR,"socket get addr info failed");
        cleanup_on_exit();
        return (EXIT_FAILURE);
    }

    if(bind(fd_soc_server, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("socket bind failed");
        syslog(LOG_ERR,"socket bind failed");
        cleanup_on_exit();
        return (EXIT_FAILURE);
    }

    /*listen for the socket connection*/
    if(listen(fd_soc_server, BACKLOG) == -1)
    {
        perror("socket listen failed");
        syslog(LOG_ERR,"socket listen failed");
        cleanup_on_exit();
        exit(EXIT_FAILURE);
    }
    memset(&ip4addr, 0, sizeof(ip4addr));
    memset(&addr_size, 0, sizeof(addr_size));

    struct client_node_s *client_node;
    SLIST_INIT(&client_list_head);
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        syslog(LOG_ERR,"Mutex init failed");
        cleanup_on_exit();
        return -1;
    }

	#ifndef AELD_CDEV
	pthread_t time_thread;
    pthread_create(&time_thread, NULL, log_timestamp_write, NULL);
	#endif


    /*super loop to poll on the data received from client*/
    while (1) 
    {
        // Accept the incoming connection
        if((fd_soc_client = accept(fd_soc_server, (struct sockaddr *)&ip4addr, &addr_size)) < 0)
        {
            syslog(LOG_ERR, "Failed to accept connection on socket");
            cleanup_on_exit();
            return (EXIT_FAILURE);
        }
        else
        {
            /*store  element buffer and  address len for each thread*/
            client_node = malloc(sizeof(struct client_node_s));
            client_node->addr_len = addr_size;
            client_node->malloc_wr_buffer = NULL;
            client_node->fd = fd_soc_client;
            client_node->addr = ip4addr;
            /*linked list for insertion at the begining of head node*/
            SLIST_INSERT_HEAD(&client_list_head, client_node, entries);

            /*create new thread*/
            if (pthread_create(&client_node->thread_id, NULL, thread_new_connection, (void*)client_node) < 0)
            {
                syslog(LOG_ERR, "Thread Creation failed");
                SLIST_REMOVE(&client_list_head, client_node, client_node_s, entries);
                close(client_node->fd);
                free(client_node);
            }
            else
            {
                pthread_join(client_node->thread_id, NULL);
            }
        }
    }

    cleanup_on_exit();
    return ret_status;
}

#ifndef AELD_CDEV
void * log_timestamp_write()
{
    time_t curr_time;
    struct tm * curr_localtime_info;
    char timestamp[TIMESTAMP_SIZE];

    sleep(10);              /*sleep for 10 sec and allow preemption*/

    while (!st_kill_thread)
    {
        time(&curr_time);
        curr_localtime_info = localtime(&curr_time);
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %T %z\n", curr_localtime_info);

        pthread_mutex_lock(&lock);
        lseek(fd_wr_file, 0, SEEK_END);
        write(fd_wr_file, timestamp, strlen(timestamp));
        pthread_mutex_unlock(&lock);

        sleep(10);  /*sleep for 10 secand allow preemption*/
    }
    return NULL;
}
#endif

void *thread_new_connection(void *client_data)
{
    struct client_node_s *client_node = (struct client_node_s*) client_data;
    int malloc_buffer_len = 0;
    int ret_status;
    int count_byte = 0;
    char char_data;
    int ret_status_retval = 0;

    char ip4addr_str[INET_ADDRSTRLEN];

  
        /*Converting IP address to string format*/
    if(NULL == inet_ntop(AF_INET, &client_node->addr.sin_addr, ip4addr_str, sizeof(ip4addr_str)))
    {
            syslog(LOG_ERR, "Failed to convert IPv4 address to string");
            cleanup_on_exit();
            return NULL;
    }

    syslog(LOG_INFO, "Accepted connection from %s", ip4addr_str);

    while (1) 
    {
        while (1)
        {
            
            if((ret_status = read_datasocket_strlocalbuf(client_node->fd, &client_node->malloc_wr_buffer, &malloc_buffer_len)) == 0)
            {
                syslog(LOG_ERR, "Failed to receive data from socket");
                close(client_node->fd);
                break;
            }
            
            /*in case of the read failure free the memory and cleint node fd*/
            if (ret_status < 0)
            {
                free(client_node->malloc_wr_buffer);
                client_node->malloc_wr_buffer = NULL;
                close(client_node->fd);
                syslog(LOG_INFO, "Closed connection from %s", ip4addr_str);
                return NULL;
            }
            pthread_mutex_lock(&lock);
            /*in case of write failure close the fd connection for the client and free the memory */
            if ((ret_status = write(fd_wr_file, client_node->malloc_wr_buffer, malloc_buffer_len)) < 0)
            {
                free(client_node->malloc_wr_buffer);
                client_node->malloc_wr_buffer = NULL;
                close(client_node->fd);
                syslog(LOG_INFO, "Closed connection from %s", ip4addr_str);
                pthread_mutex_unlock(&lock);
                return NULL;
            }
            pthread_mutex_unlock(&lock);
            break;
        }

        lseek(fd_wr_file, 0, SEEK_SET);
        /*traverse to find the total characters using coutn byte*/
        while(read(fd_wr_file, &char_data,1) > 0)
        {
            count_byte++;
        }
        if (count_byte <= 0)
        {
            ret_status = -1;
        }
            
        /*allocate the size for the malloced write buffer*/
        if (client_node->malloc_wr_buffer == NULL)
        {
            client_node->malloc_wr_buffer = (char *)malloc(sizeof(char) * count_byte);
        }
        else
        {
            client_node->malloc_wr_buffer = (char *)realloc(client_node->malloc_wr_buffer, count_byte);
        }

        lseek(fd_wr_file, 0, SEEK_SET);

        pthread_mutex_lock(&lock);
        int ret_status = read(fd_wr_file, client_node->malloc_wr_buffer, count_byte);
        pthread_mutex_unlock(&lock);

        if (ret_status == -1)
        {
            syslog(LOG_ERR, " Error while reading data from the file");
            ret_status_retval =  -1;
        }
        else
        {
            malloc_buffer_len = count_byte;
            ret_status_retval =  0;
        }

        if (ret_status_retval < 0)
        {
            printf("Error while reading from the file\n");
            free(client_node->malloc_wr_buffer);
            client_node->malloc_wr_buffer = NULL;
            close(client_node->fd);
            syslog(LOG_INFO, "Closed connection from client");
            return NULL;
        }

        ret_status = write(client_node->fd, client_node->malloc_wr_buffer, malloc_buffer_len);

        if (ret_status < 0)
        {
            syslog(LOG_ERR, "writing failure to the client");
            free(client_node->malloc_wr_buffer);
            client_node->malloc_wr_buffer = NULL;
            close(client_node->fd);
            syslog(LOG_INFO, "Closed connection from client");
            return NULL;
        }

        syslog(LOG_INFO, "Sent all packets to the client\n");

        if(client_node->malloc_wr_buffer)
            free(client_node->malloc_wr_buffer);

        client_node->malloc_wr_buffer = NULL;

        malloc_buffer_len = 0;
    }
}


void sig_handler()
{
    st_kill_thread = 1;
    syslog(LOG_INFO,"SIGINT/ SIGTERM encountered: exiting the process...");
    cleanup_on_exit();
    exit(EXIT_SUCCESS);
}

void cleanup_on_exit()
{
    close(fd_wr_file);
    close(fd_soc_server);

    struct client_node_s *client_node;

    SLIST_FOREACH(client_node, &client_list_head, entries)
    {
        if (client_node->malloc_wr_buffer)
            free(client_node->malloc_wr_buffer);
    }

    while (!SLIST_EMPTY(&client_list_head))    
    {
        client_node = SLIST_FIRST(&client_list_head);
        SLIST_REMOVE_HEAD(&client_list_head, entries);
        free(client_node);
    }

    SLIST_INIT(&client_list_head);

    pthread_mutex_destroy(&lock);
	#ifndef AELD_CDEV
		pthread_join(time_thread, NULL);
	#endif

    remove(SOCK_FILE_WRITE);

    closelog();
}



int read_datasocket_strlocalbuf(int fd_soc_client, char **malloc_buffer, int *malloc_buffer_size)
{
    int packet_size;
    char buffer[MAXDATASIZE];
    int buffer_len = 0;
    int new_linefound = 0;

    if ((buffer_len = recv(fd_soc_client, buffer, MAXDATASIZE, 0)) <= 0)
    {
        return -1;
    }

    int index = 0;
    /*traversing to find the end of buffer or new line*/
    while(index < buffer_len && buffer[index] != '\n')
    {
        index++;
    }

    if(index == buffer_len)
    {
        packet_size = buffer_len;         /*Check if there was no new line in the received buffer from socket*/
    }
    else
    {
        new_linefound = 1;
        packet_size = index + 1;        /*If a new line is found assign value of the index to packet_size*/
    }

    /*Initially mallox Buffer size is NULL perform malloc with size as packet_size*/
    if (*malloc_buffer == NULL)
    {
        if((*malloc_buffer = (char *)malloc(sizeof(char) * packet_size)) == NULL)
        {
            cleanup_on_exit();
            exit (EXIT_FAILURE);
        }
        /*perform memcopy to copy the socket recieved data to the malloced buffer*/
        memcpy(*malloc_buffer + *malloc_buffer_size, buffer, packet_size);
        *malloc_buffer_size += packet_size;                                  /*update the malloced buffer size to point*/
    }
    else
    {
        /*realloc if the buffer was already malloced to add stream of byte in to the malloced buffer*/
        if((*malloc_buffer = (char *)realloc(*malloc_buffer, *malloc_buffer_size + packet_size)) == NULL)
        {
            cleanup_on_exit();
            exit (EXIT_FAILURE);
        }

        memcpy(*malloc_buffer + *malloc_buffer_size, buffer, packet_size);
        *malloc_buffer_size += packet_size;
    }

    return new_linefound;
}
