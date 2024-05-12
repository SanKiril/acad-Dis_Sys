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
#define DATETIME_SIZE 20

const char *users_filename = "users.csv";
const char *connected_filename = "connected.csv";
const char *files_foldername = "files/";
pthread_mutex_t users_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connected_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t files_folder_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t socket_cond = PTHREAD_COND_INITIALIZER;
int socket_copied = 0;

CLIENT *clnt;  // RPC service client

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
int check_username_existence(USERNAME username) {
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
        line[strlen(line)-1] = '\0';  // fgets keeps the newline, so we need to remove it
        if (strcmp(line, username) == 0) {
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
* @brief check if user is connected (if username in connected.csv)
* @param username username to check
* @return 1 if connected, 0 otherwise
* @return -1 if error
*/
int check_user_connection(USERNAME username) {
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
int register_user(USERNAME username) {
    // check if username exists in users.csv
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 1) {
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
    USERNAME username;
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

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

/**
* @brief disconnect user, deleting it from connected.csv file and deleting username from files folder
* @param username username to disconnect
* @return 0 if successful
* @return 1 if user doesn't exist
* @return 2 if user is not connected
* @return -1 if error
*/
int disconnect_user(USERNAME username) {
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
    char modified_line[MAXLINE];
    while (fgets(line, MAXLINE, connected_file) != 0) {
        strcpy(modified_line, line);
        char *possible_username = strtok(modified_line, ";");
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

/**
* @brief disconnect operation handler. Calls disconnect_user() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_disconnect(int client_socket) {
    // get username from client socket
    USERNAME username;
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
* @brief unregister user, deleting it from users.csv file and disconnecting them if they are connected
* @param username username to unregister
* @return 0 if successful
* @return 1 if username doesn't exist
* @return -1 if error
*/
int unregister_user(USERNAME username) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // disconnect user if they are connected
    int disconnect_user_rvalue = disconnect_user(username);
    if (disconnect_user_rvalue < 0) {
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
            line[strlen(username)] = '\n';  // we need the \n back to put it into the input file
            fprintf(temp_users_file, "%s", line);
        }
    }

    // close files and mutex
    fclose(users_file);
    fclose(temp_users_file);
    remove(users_filename);
    rename("temp_users.csv", users_filename);
    pthread_mutex_unlock(&users_file_lock);

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
    USERNAME username;
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
* @brief check if filename exists in username file in files folder
* @param username username
* @param filename filename
* @return -1 if error
* @return 1 if filename exists
* @return 0 if filename doesn't exist
*/
int check_published_file_existance(USERNAME username, FILENAME filename) {
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
* @brief publish file, adding it to the username file in files folder
* @param username username
* @param filename filename
* @param description description
*/
int publish_file(USERNAME username, FILENAME filename, char description[DESCRIPTION_SIZE]) {
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

    // open username file
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

    // get datetime from client socket
    DATETIME datetime;
    if (read(client_socket, datetime, DATETIME_SIZE) < 0) {
        perror("read");
        return -1;
    }
    
    // get username from client socket
    USERNAME username;
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get filename from client socket
    FILENAME filename;
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
    } else if (publish_file_rvalue == 3) {
        // in case file has already been published
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
    } else {
        write(client_socket, "0", EXECUTION_STATUS_SIZE);
    }

    printf("OPERATION FROM %s\n", username);
        
    // send info to RPC server
    int rpc_server_result;
    if (print_file_operation_1(username, "PUBLISH", filename, datetime, &rpc_server_result, clnt) < 0) {
        clnt_perror(clnt, "list_content");
    }
    
    return 0;
}

/**
* @brief adds username, ip and port to connected.csv and creates username file in files folder
* @param client_socket socket of client
* @return 0 if successful
* @return 1 if user doesn't exist
* @return 2 if user is ubt already connected
* @return -1 if error
*/
int connect_user(USERNAME username, char ip[IP_ADDRESS_SIZE], char port[PORT_SIZE]) {
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

/**
* @brief connect operation handler. Calls connect_user() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_connect(int client_socket) {

    // get datetime from client socket
    DATETIME datetime;
    if (read(client_socket, datetime, DATETIME_SIZE) < 0) {
        perror("read");
        return -1;
    }
    
    // get username from client socket
    USERNAME username;
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
        
    // send info to RPC server
    int rpc_server_result;
    if (print_operation_1(username, "CONNECT", datetime, &rpc_server_result, clnt) < 0) {
        clnt_perror(clnt, "list_content");
    }
    
    return 0;
}

/**
* @brief deletes a file from the username
* @param client_socket socket of client
* @return 0 if successful
* @return 1 if user doesn't exist
* @return 2 if user is not connected
* @return 3 if the file has not been published by user
* @return -1 if error
*/
int delete(USERNAME username, FILENAME filename) {

    // check if user exists
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

    // check if the file has been published by the user
    int check_published_file_existance_rvalue = check_published_file_existance(username, filename);
    if (check_published_file_existance_rvalue == 0) {
        return 3;
    } else if (check_published_file_existance_rvalue < 0) {
        return -1;
    }

    // remove the file from the user file
    // open the user file in files
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "r");
    if (username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        return -1;
    }

    // temp file to edit user file
    char *temp_file = malloc(strlen(files_foldername) + strlen(username) + strlen(filename) + 3);
    asprintf(&temp_file, "%s%s_%s", files_foldername, username, filename);
    FILE *temp_username_file = fopen(temp_file, "w");
    if (temp_username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        return -1;
    }

    // go line by line in username file and check if the line contains the filename
    const long MAXLINE = 4096;
    char line[MAXLINE];
    char modified_line[MAXLINE];
    while (fgets(line, MAXLINE, username_file) != NULL) {
        strcpy(modified_line, line);
        char *possible_filename = strtok(modified_line, ";");
        if (strcmp(possible_filename, filename) != 0) {  // if line doesn't containt the filename, write it into the temp file
            fprintf(temp_username_file, "%s", line);
        }
    }

    fclose(temp_username_file);
    fclose(username_file);
    remove(username_filename);
    rename(temp_file, username_filename);
    free(temp_file);
    free(username_filename);

    pthread_mutex_unlock(&files_folder_lock);
    
    return 0;
}

/**
* @brief delete operation handler. Calls delete() and sends error code to client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_delete(int client_socket) {

    // get datetime from client socket
    DATETIME datetime;
    if (read(client_socket, datetime, DATETIME_SIZE) < 0) {
        perror("read");
        return -1;
    }
    
    // get username from client
    USERNAME username;
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get filename from client
    FILENAME filename;
    if (read(client_socket, filename, FILENAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // delete the file and send error code to client
    int delete_rvalue = delete(username, filename);
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

    // send info to RPC server
    int rpc_server_result;
    if (print_file_operation_1(username, "DELETE", filename, datetime, &rpc_server_result, clnt) < 0) {
        clnt_perror(clnt, "list_content");
    }

    return 0;
}

// user in connected.csv, with username, ip, port
struct user {
    USERNAME username;
    char ip[IP_ADDRESS_SIZE];
    char port[PORT_SIZE];
};

/**
* @brief gets all users in connected.csv and sends their info to the client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int list_users(int client_socket) {

    // get datetime from client socket
    DATETIME datetime;
    if (read(client_socket, datetime, DATETIME_SIZE) < 0) {
        perror("read");
        return -1;
    }
    
    // get username from client
    USERNAME username;
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
        return 0;
    } else if (check_username_existence_rvalue < 0) {
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
        return 0;
    } else if (check_user_connection_rvalue < 0) {
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    }

    write(client_socket, "0", EXECUTION_STATUS_SIZE);

    // open connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "r");
    if (connected_file == NULL) {
        perror("fopen");
        return -1;
    }

    int MAX_USERS = 100;
    struct user userlist[MAX_USERS];
    int usernum = 0;
    int MAXLINE = 4096;
    char line[MAXLINE];

    // get user's info from connected.csv
    while (fgets(line, MAXLINE, connected_file) != 0) {
        strcpy(userlist[usernum].username, strtok(line, ";"));
        strcpy(userlist[usernum].ip, strtok(NULL, ";"));
        strcpy(userlist[usernum].port, strtok(NULL, ";"));
        usernum++;
    }

    // send usernum to client
    char *usernum_str;
    asprintf(&usernum_str, "%d", usernum);
    sleep(0.1);
    write(client_socket, usernum_str, strlen(usernum_str));

    // send userlist to client
    for (int i = 0; i < usernum; i++) {
        sleep(0.1);
        write(client_socket, userlist[i].username, USERNAME_SIZE);
        sleep(0.1);
        write(client_socket, userlist[i].ip, IP_ADDRESS_SIZE);
        sleep(0.1);
        write(client_socket, userlist[i].port, PORT_SIZE);
    }

    fclose(connected_file);
    pthread_mutex_unlock(&connected_file_lock);

    printf("OPERATION FROM %s\n", username);

    // send info to RPC server
    int rpc_server_result;
    if (print_operation_1(username, "LIST_USERS", datetime, &rpc_server_result, clnt) < 0) {
        clnt_perror(clnt, "list_content");
    }

    return 0;
}

struct file {
    FILENAME filename;
    char description[DESCRIPTION_SIZE];
};

int list_content(int client_socket) {
    // get datetime from client socket
    DATETIME datetime;
    if (read(client_socket, datetime, DATETIME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get username from client
    USERNAME username;
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

    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
        return 0;
    } else if (check_username_existence_rvalue < 0) {
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
        return 0;
    } else if (check_user_connection_rvalue < 0) {
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    }

    // check if requested username is connected
    int check_requested_user_connection_rvalue = check_user_connection(requested_username);
    if (check_requested_user_connection_rvalue == 0) {
        write(client_socket, "2", EXECUTION_STATUS_SIZE);
        return 0;
    } else if (check_requested_user_connection_rvalue < 0) {
        write(client_socket, "3", EXECUTION_STATUS_SIZE);
        return -1;
    }

    write(client_socket, "0", EXECUTION_STATUS_SIZE);

    // open username files in files folder
    char *username_filename = malloc(strlen(files_foldername) + strlen(requested_username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, requested_username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "r");
    free(username_filename);
    if (username_file == NULL) {
        perror("fopen");
        return -1;
    }

    // get files from username file
    struct file filelist[100];
    int filenum = 0;
    int MAXLINE = 4096;
    char line[MAXLINE];
    while (fgets(line, MAXLINE, username_file) != 0) {
        if ((strcmp(line, "\n") == 0) || (strcmp(line, "") == 0)) {
            break;
        }
        strcpy(filelist[filenum].filename, strtok(line, ";"));
        strcpy(filelist[filenum].description, strtok(NULL, ";"));
        filenum++;
    }

    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    // send filenum to client
    char *filenum_str;
    asprintf(&filenum_str, "%d", filenum);
    sleep(0.1);
    write(client_socket, filenum_str, strlen(filenum_str));

    // send filelist to client
    for (int i = 0; i < filenum; i++) {
        sleep(0.1);
        write(client_socket, filelist[i].filename, FILENAME_SIZE);
        sleep(0.1);
        write(client_socket, filelist[i].description, DESCRIPTION_SIZE);
    }

    // send info to RPC server
    int rpc_server_result;
    if (print_operation_1(username, "LIST_CONTENT", datetime, &rpc_server_result, clnt) < 0) {
        clnt_perror(clnt, "list_content");
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

    OPERATION operation;
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
        if (list_users(socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "LIST_CONTENT") == 0) {
        if (list_content(socket) < 0) {
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

    // create thread for handling new connections
    pthread_attr_t threads_attr;
    pthread_attr_init(&threads_attr);
    pthread_attr_setdetachstate(&threads_attr, PTHREAD_CREATE_JOINABLE);
    pthread_t thread;

    // initiate RPC client
	clnt = clnt_create (server_ip.ip, filemanager, VERNUM, "tcp");
	if (clnt == NULL) {
		clnt_pcreateerror (server_ip.ip);
		exit (1);
	}

    // listen for new connections (allocate memory for 5 requests at a time)
    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(1);
    }

    while (1) {
        printf("s>");
        fflush(stdout);

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