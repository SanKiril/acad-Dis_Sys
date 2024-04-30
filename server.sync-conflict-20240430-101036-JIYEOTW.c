#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include "devtools.h"

#define EXECUTION_STATUS_SIZE 1
#define NUMBER_USERS_SIZE 4
#define NUMBER_FILES_SIZE 4
#define USERNAME_SIZE 256
#define FILENAME_SIZE 256
#define DESCRIPTION_SIZE 256
#define IP_ADDRESS_SIZE 16
#define PORT_SIZE 6

unsigned int check_arguments(int argc, char *argv[]) {
    // check program arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: ./server -p <port>\n");
        return -1;
    }
    if (strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: ./server -p <port>\n");
        return -1;
    }

    return atoi(argv[2]);
}

struct local_ip_info {
    int exit_code;
    char ip[IP_ADDRESS_SIZE];
};

struct local_ip_info get_local_ip() {
    // get server's local ip
    struct local_ip_info return_ip;
    return_ip.exit_code = 0;

    char hostbuffer[256];
    if (gethostname(hostbuffer, sizeof(hostbuffer)) == -1) {
        perror("gethostname");
        return_ip.exit_code = -1;
        return return_ip;
    }

    struct hostent *host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL) {
        return_ip.exit_code = -1;
        return return_ip;
    }

    strcpy(return_ip.ip, inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])));

    return return_ip;
}

int main(int argc, char* argv[]) {
    // check given port's validity
    unsigned int port_number = check_arguments(argc, argv);
    if (port_number < 1024 | port_number > 65535) {
        fprintf(stderr, "Invalid port: '%s'\n", argv[2]);
        exit(1);
    }

    // get local ip
    struct local_ip_info server_ip = get_local_ip();
    if (server_ip.exit_code < 0)
        exit(1);
    
    // init messsage
    printf("init server %s:%d\n", server_ip.ip, port_number);
}