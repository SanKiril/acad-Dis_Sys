#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "filemanager.h"

#define OPERATION_SIZE 256
#define EXECUTION_STATUS_SIZE 1
#define NUMBER_USERS_SIZE 4
#define NUMBER_FILES_SIZE 4
#define USERNAME_SIZE 256
#define FILENAME_SIZE 256
#define DESCRIPTION_SIZE 256
#define IP_ADDRESS_SIZE 16
#define PORT_SIZE 6

const char *users_filename = "users.csv";
const char *connected_filename = "connected.csv";
const char *files_foldername = "files/";

pthread_mutex_t socket_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t socket_cond = PTHREAD_COND_INITIALIZER;
int socket_copied = 0;

CLIENT *rpc_client;  // client of the RPC server

/**
* @brief check program arguments
* @param argc number of program arguments
* @param argv program arguments
* @return port number
* @return -1 if error
*/
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

// struct to hold server's local ip and error code
struct local_ip_info {
    int exit_code;
    char ip[IP_ADDRESS_SIZE];
};

/**
* @brief get server's local ip
* @return server's local ip
* @return -1 if error
*/
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

/**
* @brief register operation handler. Calls register_user() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_register(int client_socket) {
    // get username from client socket
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    int register_user_rvalue;
    // attempt to register user
    if (register_user_1(username, &register_user_rvalue, rpc_client) != RPC_SUCCESS){
        clnt_perror(rpc_client, "Call failed");
        return -1;
    } 

    // send error code to client
    if (register_user_rvalue < 0) {
        // in case there was an error
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
        return -1;
    } else if (register_user_rvalue == 1) {
        // in case username already exists
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
    } else
        write(client_socket, "0", EXECUTION_STATUS_SIZE);

    printf("OPERATION FROM %s\n", username);
    return 0;
}

/**
* @brief disconnect operation handler. Calls disconnect_user() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_disconnect(int client_socket) {
    // get username from client socket
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to disconnect user
    int disconnect_user_rvalue;
    if (disconnect_user_1(username, &disconnect_user_rvalue, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }
    
    // send error code to client
    if (disconnect_user_rvalue < 0) {
        // in case there was an error
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    } else if (disconnect_user_rvalue == 1) {
        // in case username doesn't exist
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
    } else if (disconnect_user_rvalue == 2) {
        // in case user is not connected
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
    } else
        write(client_socket, "0", EXECUTION_STATUS_SIZE);

    printf("OPERATION FROM %s\n", username);
    return 0;
}

/**
* @brief unregister operation handler. Calls unregister_user() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_unregister(int client_socket) {
    // get username from client socket
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to unregister user
    int unregister_user_rvalue;
    if (unregister_user_1(username, &unregister_user_rvalue, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }
    
    // send error code to client
    if (unregister_user_rvalue < 0) {
        // in case there was an error
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
        return -1;
    } else if (unregister_user_rvalue == 1) {
        // in case username doesn't exist
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
    } else {
        write(client_socket, "0", EXECUTION_STATUS_SIZE);
    }
    printf("OPERATION FROM %s\n", username);
    return 0;
}

/**
* @brief publish operation handler. Calls publish_file() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_publish(int client_socket) {
    // get username from client socket
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get filename from client socket
    char filename[FILENAME_SIZE];
    if (read(client_socket, filename, FILENAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get description from client socket
    char description[DESCRIPTION_SIZE];
    if (read(client_socket, description, DESCRIPTION_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to publish file
    int publish_file_rvalue;
    if (publish_file_1(username, filename, description, &publish_file_rvalue, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send error code to client
    if (publish_file_rvalue < 0) {
        // in case there was an error
        write(client_socket, "4", EXECUTION_STATUS_SIZE);
        return -1;
    } else if (publish_file_rvalue == 1) {
        // in case username doesn't exist
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
    } else if (publish_file_rvalue == 2) {
        // in case user is not connected
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
    } else if (publish_file_rvalue == 3) {
        // in case file has already been published
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
    } else {
        write(client_socket, "0", EXECUTION_STATUS_SIZE);
    }

    printf("OPERATION FROM %s\n", username);
    return 0;
}

/**
* @brief connect operation handler. Calls connect_user() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_connect(int client_socket) {
    // get username from client socket
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // save client's ip in a variable
    char client_ip[IP_ADDRESS_SIZE];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // get socket ip
    if (getpeername(client_socket, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("getpeername");
        return -1;
    }

    // convert ip to decimal dot notation
    strcpy(client_ip, inet_ntoa(addr.sin_addr));
    if (strcmp(client_ip, "0.0.0.0") == 0) {
        perror("inet_ntoa");
        return -1;
    }

    // get port from client socket
    char port[PORT_SIZE];
    if (read(client_socket, port, PORT_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to connect
    int connect_rvalue;
    if (connect_user_1(username, client_ip, port, &connect_rvalue, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send execution status
    if (connect_rvalue < 0) {
        // in case there was an error
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    } else if (connect_rvalue == 1) {
        // in case username doesn't exist
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
    } else if (connect_rvalue == 2) {
        // in case user is already connected
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
    } else {
        write(client_socket, "0", EXECUTION_STATUS_SIZE);
    }

    printf("OPERATION FROM %s\n", username);

    return 0;
}

/**
* @brief delete operation handler. Calls delete() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_delete(int client_socket) {
    // get username from client
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get filename from client
    char filename[FILENAME_SIZE];
    if (read(client_socket, filename, FILENAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to delete the file
    int delete_rvalue;
    if (delete_file_1(username, filename, &delete_rvalue, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send execution code to client
    if (delete_rvalue < 0) {
        write(client_socket, "4", EXECUTION_STATUS_SIZE);
        return -1;
    } else if (delete_rvalue == 1) {
        // in case username doesn't exist
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
    } else if (delete_rvalue == 2) {
        // in case user is not connected
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
    } else if (delete_rvalue == 3) {
        // in case file has not been published by user
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
    } else {
        write(client_socket, "0", EXECUTION_STATUS_SIZE);
    }

    printf("OPERATION FROM %s\n", username);
    return 0;
}

/**
* @brief gets all users in connected.csv and sends their info to the client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_list_users(int client_socket) {
    // get username from client
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to do get_usernum action
    struct get_usernum_rvalue get_usernum_result;
    if (get_usernum_1(username, &get_usernum_result, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send execution code to client and break if get_usernum wasn't successful
    char *execution_status_str;
    asprintf(&execution_status_str, "%d", get_usernum_result.execution_status);
    sleep(0.1);
    if (write(client_socket, execution_status_str, strlen(execution_status_str)) < 0) {
        perror("write");
        return -1;
    }
    if (get_usernum_result.execution_status < 0) {
        return -1;
    } else if (get_usernum_result.execution_status != 0) {
        return 0;
    }

    // send usernum to client
    char *usernum_str;
    asprintf(&usernum_str, "%d", get_usernum_result.usernum);
    sleep(0.1);
    if (write(client_socket, usernum_str, strlen(usernum_str)) < 0) {
        perror("write");
        return -1;
    }

    // attempt to do list_users action
    USERLIST list_users_result;
    if (list_users_1(username, &list_users_result, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send userlist to client
    for (int i = 0; i < list_users_result.USERLIST_len + 100; i++) {
        sleep(0.1);
        if (write(client_socket, list_users_result.USERLIST_val[i].username, USERNAME_SIZE) < 0) {
            perror("username write");
            return -1;
        }
        free(list_users_result.USERLIST_val[i].username);
        sleep(0.1);
        if (write(client_socket, list_users_result.USERLIST_val[i].ip, IP_ADDRESS_SIZE) < 0) {
            perror("ip write");
            return -1;
        }
        free(list_users_result.USERLIST_val[i].ip);
        sleep(0.1);
        if (write(client_socket, list_users_result.USERLIST_val[i].port, PORT_SIZE) < 0) {
            perror("port write");
            return -1;
        }
        free(list_users_result.USERLIST_val[i].port);
    }

    printf("OPERATION FROM %s\n", username);
    return 0;
}

int handle_list_content(int client_socket) {
    // get username from client
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get requested username from client
    char requested_username[USERNAME_SIZE];
    if (read(client_socket, requested_username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to do get_filenum action
    struct get_filenum_rvalue get_filenum_result;
    if (get_filenum_1(username, requested_username, &get_filenum_result, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send execution code to client and break if get_filenum wasn't successful
    write(client_socket, (char *)&get_filenum_result.execution_status, EXECUTION_STATUS_SIZE);
    if (get_filenum_result.execution_status < 0) {
        return -1;
    } else if (get_filenum_result.execution_status != 0) {
        return 0;
    }

    // send filenum to client
    char *filenum_str;
    asprintf(&filenum_str, "%d", get_filenum_result.filenum);
    sleep(0.1);
    write(client_socket, filenum_str, strlen(filenum_str));

    // attempt to do list_content action
    FILELIST list_content_result;
    if (list_content_1(username, requested_username, &list_content_result, rpc_client) != RPC_SUCCESS) {
        clnt_perror(rpc_client, "Call failed");
        return -1;
    }

    // send filelist to client
    for (int i = 0; i < list_content_result.FILELIST_len; i++) {
        sleep(0.1);
         if (write(client_socket, list_content_result.FILELIST_val[i].filename, FILENAME_SIZE) < 0) {
             perror("write");
             return -1;
         }
        free(list_content_result.FILELIST_val[i].filename);
        sleep(0.1);
        if (write(client_socket, list_content_result.FILELIST_val[i].description, DESCRIPTION_SIZE) < 0) {
            perror("write");
            return -1;
        }
        free(list_content_result.FILELIST_val[i].description);
    }

    printf("OPERATION FROM %s\n", username);
    return 0;
}

/**
* @brief thread function to handle petition from client, calling the specific handler
* @param client_socket client socket
*/
void petition_handler(void *client_socket) {
    // get petition from client socket

    if (socket_copied == 1)
        pthread_exit(NULL); 
    pthread_mutex_lock(&socket_lock);
    int socket = *(int *)client_socket;
    socket_copied = 1;
    pthread_cond_signal(&socket_cond);
    pthread_mutex_unlock(&socket_lock);

    char operation[OPERATION_SIZE];
    if (read(socket, operation, OPERATION_SIZE) < 0) {
        perror("read");
        pthread_exit(NULL);
    }

    // handle petition, calling the specific handler
    if (strcmp(operation, "REGISTER") == 0) {
        if (handle_register(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "UNREGISTER") == 0) {
        if (handle_unregister(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "CONNECT") == 0) {
        if (handle_connect(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "PUBLISH") == 0) {
        if (handle_publish(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "DISCONNECT") == 0) {
        if (handle_disconnect(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "DELETE") == 0) {
        if (handle_delete(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "LIST_USERS") == 0) {
        if (handle_list_users(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "LIST_CONTENT") == 0) {
        if (handle_list_content(socket) < 0) {
            pthread_exit(NULL);
        }
    } else {
        printf("INCORRECT OPERATION\n");
        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

/**
* @brief handle SIGINT, closing every mutex before exiting
*/
void handle_sigint() {
    // delete socket mutex
    pthread_mutex_destroy(&socket_lock);

    exit(0);
}

int main(int argc, char* argv[]) {
    // register signal handler
    signal(SIGINT, handle_sigint);

    // check given port's validity
    unsigned int port_number = check_arguments(argc, argv);
    if ((port_number < 1024) | (port_number > 65535)) {
        fprintf(stderr, "Invalid port: '%s'\n", argv[2]);
        exit(1);
    }

    // get local ip
    struct local_ip_info server_ip = get_local_ip();
    if (server_ip.exit_code < 0)
        exit(1);
    
    // init messsage
    printf("init server %s:%d\n", server_ip.ip, port_number);

    // create/clear users.csv
    FILE *tuples_file = fopen(users_filename, "w");
    if (tuples_file == NULL || fclose(tuples_file) < 0) {
        perror("fopen");
        exit(1);
    }

    // create/clear connected.csv
    FILE *connected_users_file = fopen(connected_filename, "w");
    if (connected_users_file == NULL || fclose(connected_users_file) < 0) {
        perror("fopen");
        exit(1);
    }

    // empty files folder
    DIR *files_folder = opendir(files_foldername);
    if (files_folder == NULL) {
        perror("opendir");
        exit(1);
    }
    struct dirent *file;
    while ((file = readdir(files_folder)) != NULL) {
        if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {
            char *file_path = malloc(strlen(files_foldername) + strlen(file->d_name) + 2);
            strcpy(file_path, files_foldername);
            strcat(file_path, file->d_name);
            remove(file_path);
            free(file_path);
        }
    }
    closedir(files_folder);

    // generate server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    // set socket options to reuse port (for easier debugging)
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // bind socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        exit(1);
    }

    // create thread
    pthread_attr_t threads_attr;
    pthread_attr_init(&threads_attr);
    pthread_attr_setdetachstate(&threads_attr, PTHREAD_CREATE_JOINABLE);
    pthread_t thread;  // thread for handling new connections

    // init RPC client
    rpc_client = clnt_create(server_ip.ip, filemanager, VERNUM, "tcp");
    if (rpc_client == NULL) {
        clnt_pcreateerror(server_ip.ip);
        exit(1);
    }

    while (1) {
        printf("s>");
        fflush(stdout);
        
        // listen for new connections
        if (listen(server_socket, 5) < 0) {
            perror("listen");
            exit(1);
        }

        // accept new connections
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("accept");
            exit(1);
        }

        // create thread
        pthread_mutex_lock(&socket_lock);
        socket_copied = 0;
        if (pthread_create(&thread, &threads_attr, (void *) petition_handler, (void *) &client_socket) < 0) {
            perror("pthread_create");
            exit(1);
        }
        while (socket_copied == 0)
            pthread_cond_wait(&socket_cond, &socket_lock);
        pthread_mutex_unlock(&socket_lock);
        
        // ensure the system knows that the thread can be cleaned up automatically without the server waiting
        if (pthread_join(thread, NULL) < 0) {
            // doing this for good practice, pthread_join on a detached thread always returns success
            perror("pthread_join");
            exit(1);  
        }

        // close client socket
        if (close(client_socket) < 0) {
            perror("close");
            exit(1);
        }
    }
}