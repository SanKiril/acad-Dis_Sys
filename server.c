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

#include "devtools.h"

#define OPERATION_SIZE 256
#define EXECUTION_STATUS_SIZE 1
#define NUMBER_USERS_SIZE 4
#define NUMBER_FILES_SIZE 4
#define USERNAME_SIZE 256
#define FILENAME_SIZE 256
#define DESCRIPTION_SIZE 256
#define IP_ADDRESS_SIZE 16
#define PORT_SIZE 6

int socket_copied = 0;
const char *users_filename = "users.csv";
const char *connected_filename = "connected.csv";
const char *files_foldername = "files/";
pthread_mutex_t users_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connected_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t files_folder_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_lock = PTHREAD_MUTEX_INITIALIZER;

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
* @brief check if username exists in users.csv
* @param username username to check
* @return 1 if exists, 0 otherwise
* @return -1 if error
*/
int check_username_existence(char username[USERNAME_SIZE]) {
    // open users.csv file
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "r");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        return -1;
    }

    // go line by line, checking if the line matches the username
    const long MAXLINE = 4096;
    char line[MAXLINE];
    while (fgets(line, MAXLINE, users_file) != NULL) {
        if (strstr(line, username) != NULL) {
            // username exists
            fclose(users_file);
            pthread_mutex_unlock(&users_file_lock);
            return 1;
        }
    }

    // username doesn't exist
    fclose(users_file);
    pthread_mutex_unlock(&users_file_lock);
    return 0;
}

/**
* @brief check if username exists in files folder
* @param username username to check
* @return 1 if exists, 0 otherwise
* @return -1 if error
*/
int check_username_file_existance(char username[USERNAME_SIZE]) {
    // open files folder
    pthread_mutex_lock(&files_folder_lock);
    DIR *dir = opendir(files_foldername);
    if (dir == NULL) {
        perror("opendir");
        pthread_mutex_unlock(&files_folder_lock);
        return -1;
    }

    // check if username is one of the files in the folder
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, username) == 0) {
            // username exists
            closedir(dir);
            pthread_mutex_unlock(&files_folder_lock);
            return 1;
        }
    }
    
    // username doesn't exist
    closedir(dir);
    pthread_mutex_unlock(&files_folder_lock);
    return 0;
}

/**
* @brief check if user is connected (if username in connected.csv)
* @param username username to check
* @return 1 if connected, 0 otherwise
* @return -1 if error
*/
int check_user_connection(char username[USERNAME_SIZE]) {
    // open connected file
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "r");
    if (connected_file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&connected_file_lock);
        return -1;
    }

    // go line by line, checking if the line's username matches the username
    const long MAXLINE = 4096;
    char line[MAXLINE];
    while (fgets(line, MAXLINE, connected_file) != 0) {
        char *possible_username = strtok(line, ";");
        if (strcmp(possible_username, username) == 0) {
            // username exists
            fclose(connected_file);
            pthread_mutex_unlock(&connected_file_lock);
            return 1;
        }
    }

    // username doesn't exist
    fclose(connected_file);
    pthread_mutex_unlock(&connected_file_lock);
    return 0;
}

/**
* @brief register user, adding it to users.csv file
* @param username username to register
* @return 0 if successful
* @return 1 if username already exists
* @return -1 if error
*/
int register_user(char username[USERNAME_SIZE]) {
    // check if username exists in users.csv
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 1) {
        fprintf(stderr, "Username already exists.\n");
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // open users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "a");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        return -1;
    }

    // write username to users.csv
    fprintf(users_file, "%s\n", username);
    fclose(users_file);
    pthread_mutex_unlock(&users_file_lock);

    return 0;
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

    printf("%s\n", username);
    fflush(stdout);

    // attempt to register user
    int register_user_rvalue = register_user(username);
    
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

int disconnect_user(char username[USERNAME_SIZE]) {
    // check is user exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        return 2;
    } else if (check_user_connection_rvalue < 0) {
        return -1;
    }

    // delete username line from connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "r+");
    if (connected_file == NULL) {
        pthread_mutex_unlock(&connected_file_lock);
        perror("fopen");
        return -1;
    }
    int MAXLINE = 4096;
    char line[MAXLINE];
    FILE *temp_connected_file = fopen("temp_connected.csv", "w");
    if (temp_connected_file == NULL) {
        pthread_mutex_unlock(&connected_file_lock);
        perror("fopen");
        return -1;
    }
    while (fgets(line, MAXLINE, connected_file) != 0) {
        char *possible_username = strtok(line, ";");
        if (strcmp(possible_username, username) != 0) {  // if user is not the line's username, write line into temp_file
            fprintf(temp_connected_file, "%s", line);
        }
    }
    fclose(connected_file);
    fclose(temp_connected_file);
    remove(connected_filename);
    rename("temp_connected.csv", connected_filename);
    pthread_mutex_unlock(&connected_file_lock);

    // delete username file from files folder
    pthread_mutex_lock(&files_folder_lock);
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    remove(username_filename);
    pthread_mutex_unlock(&files_folder_lock);
    free(username_filename);
    
    return 0;
}

int handle_disconnect(int client_socket) {
    // get username from client socket
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // attempt to disconnect user
    int disconnect_user_rvalue = disconnect_user(username);
    
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
* @brief unregister user, deleting it from users.csv file and deleting its folder
* @param username username to unregister
* @return 0 if successful
* @return 1 if username doesn't exist
* @return -1 if error
*/
int unregister_user(char username[USERNAME_SIZE]) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // delete username from users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "r");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        return -1;
    }

    FILE *temp_users_file = fopen("temp_users.csv", "w");
    if (temp_users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        return -1;
    }

    char line[USERNAME_SIZE];
    while (fgets(line, USERNAME_SIZE, users_file) != NULL) {
        line[strlen(username)] = '\0';  // fgets includes \n in buffer, we don't want that
        if (strcmp(line, username) != 0) {
            fprintf(temp_users_file, "%s", line);
        }
    }

    // close files and mutex
    fclose(users_file);
    fclose(temp_users_file);
    remove(users_filename);
    rename("temp_users.csv", users_filename);
    pthread_mutex_unlock(&users_file_lock);

    // disconnect user in case they were connected
    disconnect_user(username);

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
    int unregister_user_rvalue = unregister_user(username);
    
    
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
* @brief check if filename exists in username
* @param username username
* @param filename filename
* @return -1 if error
* @return 1 if filename exists
* @return 0 if filename doesn't exist
*/
int check_published_file_existance(char username[USERNAME_SIZE], char filename[FILENAME_SIZE]) {
    // open username
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "r");
    free(username_filename);
    if (username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        return -1;
    }

    // check if filename is in any line in username
    const long MAXLINE = 4096;
    char line[MAXLINE];
    while (fgets(line, MAXLINE, username_file) != NULL) {
        char *possible_filename = strtok(line, ";");
        if (strcmp(possible_filename, filename) == 0) {
            // filename exists
            fclose(username_file);
            pthread_mutex_unlock(&files_folder_lock);
            return 1;
        }
    }

    // filename doesn't exist
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);
    return 0;
}

/**
* @brief publish file to username
* @param username username
* @param filename filename
* @param description description
*/
int publish_file(char username[USERNAME_SIZE], char filename[FILENAME_SIZE], char description[DESCRIPTION_SIZE]) {
    // check if user is registered
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        return 2;
    } else if (check_user_connection_rvalue < 0) {
        return -1;
    }
    
    // check if file has been published by user
    int check_file_existance_rvalue = check_published_file_existance(username, filename);
    if (check_file_existance_rvalue == 1) {
        return 3;
    } else if (check_file_existance_rvalue < 0) {
        return -1;
    }

    // open username
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "a");
    free(username_filename);
    if (username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        return -1;
    }

    // write filename;description at the end of the file
    fprintf(username_file, "%s;%s\n", filename, description);
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

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
    int publish_file_rvalue = publish_file(username, filename, description);
    
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
    } else {
        write(client_socket, "0", EXECUTION_STATUS_SIZE);
    }

    printf("OPERATION FROM %s\n", username);
    return 0;
}

int connect_user(char username[USERNAME_SIZE], char ip[IP_ADDRESS_SIZE], char port[PORT_SIZE]) {
    // check if user is registered
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // check if user is connected already
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 1) {
        return 2;
    } else if (check_user_connection_rvalue < 0) {
        return -1;
    }

    // write client's username, ip and port to connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "a");
    if (connected_file == NULL) {
        pthread_mutex_unlock(&connected_file_lock);
        perror("fopen");
        return -1;
    }
    fprintf(connected_file, "%s;%s;%s\n", username, ip, port);
    fclose(connected_file);
    pthread_mutex_unlock(&connected_file_lock);

    // create or clear username file in files folder
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "w");
    free(username_filename);
    if (username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        return -1;
    }
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    return 0;
}

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
    int connect_rvalue = connect_user(username, client_ip, port);

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
* @brief thread function to handle petition from client, calling the specific handler
* @param client_socket client socket
*/
void petition_handler(void *client_socket) {
    // get petition from client socket

    pthread_mutex_lock(&socket_lock);
    int socket = *(int *)client_socket;
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
    }

    pthread_exit(NULL);
}

/**
* @brief handle SIGINT, closing every mutex before exiting
*/
void handle_sigint() {
    // delete all mutexes
    pthread_mutex_destroy(&users_file_lock);
    pthread_mutex_destroy(&files_folder_lock);
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

    // init mutexes and create thread
    pthread_mutex_init(&users_file_lock, NULL);  // mutex to ensure users.csv is not a race condition
    pthread_mutex_init(&connected_file_lock, NULL);  // mutex to ensure connected.csv is not a race condition
    pthread_mutex_init(&files_folder_lock, NULL);  // mutex to ensure files folder is not a race condition
    pthread_mutex_init(&socket_lock, NULL);  // mutex to ensure socket is not a race condition

    pthread_attr_t threads_attr;
    pthread_attr_init(&threads_attr);
    pthread_attr_setdetachstate(&threads_attr, PTHREAD_CREATE_JOINABLE);
    pthread_t thread;  // thread for handling new connections

    while (1) {
        // listen for new connections
        printf("s>");
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
        if (pthread_create(&thread, &threads_attr, (void *) petition_handler, (void *) &client_socket) < 0) {
            perror("pthread_create");
            exit(1);
        }

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