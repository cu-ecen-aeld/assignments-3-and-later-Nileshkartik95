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

#define BACKLOG                 (10)                        /* how many pending connections queue will hold*/
#define MAXDATASIZE             (100)                       /* max buffer size*/
#define SOCK_FILE_WRITE         ("/var/tmp/aesdsocketdata") /* temp file to write aesd socket data*/


/***********************************Global Variable***************************/
char *malloc_wr_buffer = NULL;
int fd_wr_file;
int fd_soc_server;
char *malloc_buffer = NULL;
int close_cli = 0;
int malloc_buffer_size = 0;
/*****************************Function declaration****************************/
void cleanup_on_exit();
void sig_handler();
int read_datasocket_strlocalbuf(int file_desc, char *buffer, int buff_len);

/*****************************Function definition******************************/

int main(int argc, char* argv[])
{
    int fd_soc_client;
    struct addrinfo hints;
    struct addrinfo *res;
    char buf[MAXDATASIZE];
    struct sockaddr_in ip4addr;
    socklen_t addr_size;
    int ret_val = 0;
    char client_addr[INET_ADDRSTRLEN];
    int opt_val = 1;
    int deamon_enabled = 0;
    int recv_bytes = 0;
    int buffer_size = 0;

    openlog(NULL, LOG_CONS | LOG_PID | LOG_PERROR , LOG_USER);									/*open connection for sys logging, ident is NULL to use this Program for the user level messages*/

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

    /*manipulating set option*/
    if(setsockopt(fd_soc_server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt_val, sizeof(int)) == -1)
    {
        perror("set socket opt failed");
        syslog(LOG_ERR,"set socket opt failed");
        cleanup_on_exit();
        return (EXIT_FAILURE);
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
    memset(&ip4addr, 0, sizeof(struct sockaddr_in));
    memset(&addr_size, 0, sizeof(socklen_t));
    /*super loop to poll on the data received from client*/
    while (1)
    {
        if((fd_soc_client = accept(fd_soc_server, (struct sockaddr *)&ip4addr, &addr_size)) < 0)
        {
            syslog(LOG_ERR, "Failed to accept connection on socket");
            cleanup_on_exit();
            return (EXIT_FAILURE);
        }

        /*Converting IP address to string format*/
        if(NULL == inet_ntop(AF_INET, &ip4addr.sin_addr, client_addr, sizeof(client_addr)))
        {
            syslog(LOG_ERR, "Failed to convert IPv4 address to string");
            cleanup_on_exit();
            return (EXIT_FAILURE);
        }
        syslog(LOG_INFO, "Accepted connection from %s", client_addr);           /*print the accepted client IPv4 Address*/

        while(1)
        {
            while(1)
            {
                if((buffer_size = recv(fd_soc_client, buf, MAXDATASIZE, 0)) <= 0 )
                {
                    syslog(LOG_ERR, "Failed to receive data from socket");
                    close(fd_soc_client);
                    break;
                }

                if((ret_val = read_datasocket_strlocalbuf(fd_wr_file, buf, buffer_size)) == -1)
                {
                    close(fd_soc_client);               /*if read failed*/
                    break;
                }

                if (ret_val > 0)
                {
                    recv_bytes += ret_val;
                    break;
                }
            }

            if (malloc_wr_buffer != NULL)
            {
                free(malloc_wr_buffer);
            }

            if((malloc_wr_buffer = (char *)malloc(sizeof(char) * recv_bytes)) == NULL)
            {
                syslog(LOG_ERR, "Malloc Failure");
                cleanup_on_exit();
                return (EXIT_FAILURE);
            }

            lseek(fd_wr_file, 0, SEEK_SET);

            if(((ret_val = read(fd_wr_file, malloc_wr_buffer, recv_bytes))== -1) || (ret_val != recv_bytes))
            {
                close(fd_soc_client);
                syslog(LOG_INFO, "Read Failure: closing connection from %s", client_addr);
                break;
            }
            else
            {
                syslog(LOG_INFO, "Read %d bytes from the file%s", ret_val, client_addr);
            }

            if((buffer_size = write(fd_soc_client, malloc_wr_buffer, recv_bytes)) == recv_bytes)
            {
                syslog(LOG_INFO, "Write Complete to client");
            }
            else
            {
                close(fd_soc_client);
                syslog(LOG_INFO, "Error sending data to client");
                break;
            }
        }
    }
    cleanup_on_exit();
    return 0;

}



void sig_handler()
{
    syslog(LOG_INFO,"SIGINT/ SIGTERM encountered: exiting the process...");
    cleanup_on_exit();
    exit(EXIT_SUCCESS);
}


void cleanup_on_exit()
{

    close(fd_wr_file);
    close(fd_soc_server);

    if(malloc_wr_buffer != NULL)
    {
        free(malloc_wr_buffer);
    }

    if(remove(SOCK_FILE_WRITE) == -1)
    {
        syslog(LOG_ERR,"Socket file removal failed");
    }

}

int read_datasocket_strlocalbuf(int file_desc, char *buffer, int buff_len)
{

    int new_line_found = 0;
    int index = 0;
    int packet_size = 0;
    int wr_buffer_len = 0;

    /*traversing to find the end of buffer or new line*/
    while(index < buff_len && buffer[index] != '\n')
    {
        index++;
    }

    if(index == buff_len)
    {
        packet_size = buff_len;         /*Check if there was no new line in the received buffer from socket*/
    }
    else
    {
        new_line_found = 1;
        packet_size = index + 1;        /*If a new line is found assign value of the index to packet_size*/
    }

    /*Initially mallox Buffer size is NULL perform malloc with size as packet_size*/
    if (malloc_buffer == NULL)
    {
        if((malloc_buffer = (char *)malloc(sizeof(char) * packet_size)) == NULL)
        {
            cleanup_on_exit();
            exit (EXIT_FAILURE);
        }
        /*perform memcopy to copy the socket recieved data to the malloced buffer*/
        memcpy(malloc_buffer + malloc_buffer_size, buffer, packet_size);
        malloc_buffer_size += packet_size;                                  /*update the malloced buffer size to point*/
    }
    else
    {
        /*realloc if the buffer was already malloced to add stream of byte in to the malloced buffer*/
        if((malloc_buffer = (char *)realloc(malloc_buffer, (malloc_buffer_size + packet_size))) == NULL)
        {
            cleanup_on_exit();
            exit (EXIT_FAILURE);
        }

        memcpy(malloc_buffer + malloc_buffer_size, buffer, packet_size);
        malloc_buffer_size += packet_size;
    }

    /*if a new line is found write in the file_desc untill the new line character is found*/
    if (new_line_found == 1)
    {
        wr_buffer_len = malloc_buffer_size;
        write(file_desc, malloc_buffer, malloc_buffer_size);
        malloc_buffer_size = 0;                 /*reset the static malloc_buffer_size*/
        free(malloc_buffer);                    /*free the malloced buffer*/
        malloc_buffer = NULL;

        return wr_buffer_len;
    }

    return wr_buffer_len;
}
