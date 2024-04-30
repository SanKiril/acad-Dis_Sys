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

const char *users_filename = "users.csv";
const char *connected_filename = "connected.csv";
const char *files_foldername = "files/";
const char *files_filename = "files.csv";
pthread_mutex_t users_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t files_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_lock = PTHREAD_MUTEX_INITIALIZER;

/**
* @brief remove folder and its content recursively
* @param username username to remove
* @return 0 if successful
* @return -1 if error
*/
int remove_folder(const char *foldername) {
    // check if content inside folder is a file
    struct dirent *ent;
    DIR *dir = opendir(foldername);
    if (dir == NULL) {
        // the folder doesn't exist
        return 1;
    }
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG) {
            // if it is a file, just remove it
            if (unlink(ent->d_name) < 0) {
                perror("unlink");  // like remove but only works with files
                return -1;
            }
        } else {
            // if it is a folder, recursively remove its content
            remove_folder(ent->d_name);
        }
    }
    closedir(dir);

    // now that the folder is empty, remove it
    if (rmdir(foldername) < 0) {
        perror("rmdir");
        return -1;
    }

    return 0;
}

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
int check_username_existence(char *username) {
    // open users file
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
* @brief check if username folder exists in files/
* @param username username to check
* @return 1 if exists, 0 otherwise
* @return -1 if error
*/
int check_username_folder_existence(char *username) {
    // get names of folders inside files
    DIR *dir = opendir(files_foldername);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, username) == 0) {
            // username exists
            closedir(dir);
            return 1;
        }
    }

    // username doesn't exist
    closedir(dir);
    return 0;
}

int check_user_connection(char *username) {
    // open connected file
    FILE *connected_file = fopen(connected_filename, "r");
    if (connected_file == NULL) {
        perror("fopen");
        return -1;
    }

    // go line by line, checking if the line's username matches the username
    const long MAXLINE = 4096;
    char line[MAXLINE];
    while (fgets(line, MAXLINE, connected_file) != NULL) {
        char *possible_username = strtok(line, ";");
        if (strcmp(possible_username, username) == 0) {
            // username exists
            fclose(connected_file);
            return 1;
        }
    }

    // username doesn't exist
    fclose(connected_file);
    return 0;
}

/**
* @brief register user, adding it to users.csv file
* @param username username to register
* @return 0 if successful
* @return 1 if username already exists
* @return -1 if error
*/
int register_user(char *username) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 1) {
        fprintf(stderr, "Username already exists.\n");
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // open users file
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "a");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        return -1;
    }
    pthread_mutex_unlock(&users_file_lock);

    // write username
    fprintf(users_file, "%s\n", username);
    fclose(users_file);

    // generate folder with username as name
    char *user_folder = malloc(strlen(files_foldername) + strlen(username) + 2);
    strcpy(user_folder, files_foldername);
    strcat(user_folder, username);
    if (mkdir(user_folder, 0777) < 0) {
        perror("mkdir");
        return -1;
    }
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
        printf("OPERATION FROM %s\n", username);
        return 0;
    } 

    // generate user folder
    char *user_folder = malloc(strlen(files_foldername) + strlen(username) + 2);
    strcpy(user_folder, files_foldername);
    strcat(user_folder, username);
    if (mkdir(user_folder, 0777) < 0) {
        free(user_folder);
        perror("mkdir");
        return -1;
    }
    free(user_folder);

    // create files.csv inside user folder
    char *user_files_filename = malloc(strlen(files_foldername) + strlen(username) + strlen(files_filename) + 2);
    strcpy(user_files_filename, files_foldername);
    strcat(user_files_filename, username);
    strcat(user_files_filename, "/");
    strcat(user_files_filename, files_filename);
    FILE *files_file = fopen(user_files_filename, "w");
    free(user_files_filename);
    if (files_file == NULL) {
        perror("fopen");
        return -1;
    }
    fclose(files_file);

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
int unregister_user(char *username) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        return 1;
    } else if (check_username_existence_rvalue < 0) {
        return -1;
    }

    // delete username from users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "r+");
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
        if (strcmp(line, username) != 0) {
            fprintf(temp_users_file, "%s", line);
        }
    }
    fclose(users_file);
    fclose(temp_users_file);
    remove(users_filename);
    rename("temp_users.csv", users_filename);
    pthread_mutex_unlock(&users_file_lock);

    // delete user folder
    char *user_folder = malloc(strlen(files_foldername) + strlen(username) + 2);
    strcpy(user_folder, files_foldername);
    strcat(user_folder, username);
    if (remove_folder(user_folder) < 0) {
        free(user_folder);
        return -1;
    }
    free(user_folder);

    // TODO: check if username is in connected.csv, and if so delete username from it too

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
* @brief publish operation handler. Sends error code to client if necessary
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int handle_publish(int client_socket) {
    // get client username
    char username[USERNAME_SIZE];
    if (read(client_socket, username, USERNAME_SIZE) < 0) {
        perror("read");
        return -1;
    }
    
    // check if user is registered
    if (check_username_existence(username) < 0) {
        write(client_socket, "1", EXECUTION_STATUS_SIZE);
        return 0;
    }

    // TODO: check if user is connected, and if not send error code 2

    // get filename from client socket
    char filename[FILENAME_SIZE];
    if (read(client_socket, filename, FILENAME_SIZE) < 0) {
        perror("read");
        return -1;
    }

    // get file info from client socket
    char file_info[DESCRIPTION_SIZE];
    if (read(client_socket, file_info, DESCRIPTION_SIZE) < 0) {
        perror("read");
        return -1;
    }
    
    // check if user folder exists
    if (check_username_folder_existence(username) < 0) {
        fprintf(stderr, "User folder doesn't exist.\n");
        return -1;
    }

    // check if files.csv exists inside user folder
    char *files_info_file = malloc(strlen(files_foldername) + strlen(username) + strlen(files_filename) + 3);
    strcpy(files_info_file, files_foldername);
    strcat(files_info_file, username);
    strcat(files_info_file, "/");
    strcat(files_info_file, files_filename);
    FILE *files_info = fopen(files_info_file, "r");
    free(files_info_file);
    if (files_info == NULL) {
        // files.csv doesn't exist, send error code to client
        write(client_socket, "4", EXECUTION_STATUS_SIZE);
        fprintf(stderr, "Error: files.csv doesn't exist inside user folder.\n");
        return 0;
    }

    // see if filename is already in files.csv
    const long MAXLINE = 4096;
    char line [MAXLINE];
    while (fgets(line, MAXLINE, files_info) != NULL) {
        char *possible_filename = strtok(line, ";");
        if (strcmp(possible_filename, filename) == 0) {
            // file already exists, send error code to client
            fclose(files_info);
            write(client_socket, "3", EXECUTION_STATUS_SIZE);
            return 0;
        }
    }

    // add filename to files.csv
    fclose(files_info);
    files_info_file = malloc(strlen(files_foldername) + strlen(username) + strlen(files_filename) + 3);
    strcpy(files_info_file, files_foldername);
    strcat(files_info_file, username);
    strcat(files_info_file, "/");
    strcat(files_info_file, files_filename);
    FILE *files_file = fopen(files_info_file, "a");
    if (files_file == NULL) {
        write(client_socket, "4", EXECUTION_STATUS_SIZE);
        perror("fopen");
        return -1;
    }
    fprintf(files_file, "%s;%s\n", filename, file_info);
    fclose(files_file);

    // add file to users folder
    char *file_path = malloc(strlen(files_foldername) + strlen(username) + strlen(filename) + 3);
    strcpy(file_path, files_foldername);
    strcat(file_path, username);
    strcat(file_path, "/");
    strcat(file_path, filename);
    FILE *file = fopen(file_path, "w");
    free(file_path);
    if (file == NULL) {
        perror("fopen");
        write(client_socket, "4", EXECUTION_STATUS_SIZE);
        return -1;
    }
    fclose(file);

    write(client_socket, "0", EXECUTION_STATUS_SIZE);
    printf("OPERATION FROM %s\n", username);
    return 0;

}

/**
* @brief thread function to handle petition from client, calling the specific handler
* @param client_socket client socket
*/
void petition_handler(int client_socket) {
    // get petition from client socket

    pthread_mutex_lock(&socket_lock);
    int socket = client_socket;
    pthread_mutex_unlock(&socket_lock);

    char operation[OPERATION_SIZE];
    if (read(socket, operation, OPERATION_SIZE) < 0) {
        perror("read");
        pthread_exit(NULL);
    }

    // handle petition, calling the specific handler
    if (strcmp(operation, "REGISTER") == 0) {
        if (handle_register(client_socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "UNREGISTER") == 0) {
        if (handle_unregister(client_socket) < 0) {
            pthread_exit(NULL);
        }
    } else if (strcmp(operation, "PUBLISH") == 0) {
        if (handle_publish(client_socket) < 0) {
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
    pthread_mutex_destroy(&files_file_lock);
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

    // clear files folder
    if (remove_folder(files_foldername) < 0) {
        exit(1);
    }

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
    pthread_mutex_init(&files_file_lock, NULL);  // mutex to ensure files.csv is not a race condition
    pthread_mutex_init(&socket_lock, NULL);  // mutex to ensure socket is not a race condition
    pthread_attr_t threads_attr;
    pthread_attr_init(&threads_attr);
    pthread_attr_setdetachstate(&threads_attr, PTHREAD_CREATE_DETACHED);
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