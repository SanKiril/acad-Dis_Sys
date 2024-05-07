#include "soapH.h"
#include "soap.nsmap"
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

// files and directories
const char *users_filename = "users.csv";
const char *connected_filename = "connected.csv";
const char *files_foldername = "files/";

// mutex locks
pthread_mutex_t users_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connected_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t files_folder_lock = PTHREAD_MUTEX_INITIALIZER;

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

int main (int argc, char *argv[]) {
    int server_socket, client_socket;  // sockets del cliente y servidor
    struct soap soap;

    // get port and local ip
    int port = check_arguments(argc, argv);
    if (port < 0)
        exit(1);

    struct local_ip_info server_ip = get_local_ip();
    if (server_ip.exit_code < 0)
        exit(1);

    printf("init server %s:%d\n", server_ip.ip, port);

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
            char *file_path = malloc(strlen(files_foldername) + strlen(file->d_name) + 2);  // path of the file to remove
            strcpy(file_path, files_foldername);
            strcat(file_path, file->d_name);

            remove(file_path);
            free(file_path);
        }
    }
    closedir(files_folder);

    // init mutexes
    pthread_mutex_init(&users_file_lock, NULL);  // mutex to ensure users.csv is not a race condition
    pthread_mutex_init(&connected_file_lock, NULL);  // mutex to ensure connected.csv is not a race condition
    pthread_mutex_init(&files_folder_lock, NULL);  // mutex to ensure files folder is not a race condition

    soap_init(&soap);

    server_socket = soap_bind(&soap, NULL, port, 100);
    if (server_socket < 0) {
        soap_print_fault(&soap, stderr);
        exit(-1);
    }

    while(1) {
        printf("s>");
        fflush(stdout);

        // get new connections from clients
        client_socket = soap_accept(&soap);
        if (client_socket < 0) {
            soap_print_fault(&soap, stderr);
            exit(1);
        }

        // serve service to clients
        soap_serve(&soap);
        soap_end(&soap);
    }

    return 0;
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
int register_user(struct soap*, USERNAME username, int *res) {
    // check if username exists in users.csv
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 1) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // open users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "a");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    // write username to users.csv
    fprintf(users_file, "%s\n", username);
    fclose(users_file);
    pthread_mutex_unlock(&users_file_lock);

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
}

int disconnect_backend(USERNAME username) {
    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        return 2;
    } else if (check_user_connection_rvalue < 0) {
        return -1;
    }

    // delete username from connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "r+");
    if (connected_file == NULL) {
        pthread_mutex_unlock(&connected_file_lock);
        perror("fopen");
        return -1;
    }
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
* @brief disconnect user, deleting it from connected.csv file and deleting username from files folder
* @param username username to disconnect
* @return 0 if successful
* @return 1 if user doesn't exist
* @return 2 if user is not connected
* @return -1 if error
*/
int disconnect_user(struct soap*, USERNAME username, int *res) {
    // check if user exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // attempt to disconnect user
    int disconnect_backend_rvalue = disconnect_backend(username);
    if (disconnect_backend_rvalue != 0) {
        *res = disconnect_backend_rvalue;
        return SOAP_OK;
    } 
    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
}

/**
* @brief unregister user, deleting it from users.csv file and disconnecting them if they are connected
* @param username username to unregister
* @return 0 if successful
* @return 1 if username doesn't exist
* @return -1 if error
*/
int unregister_user(struct soap*, USERNAME username, int *res) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // disconnect user if they are connected
    int disconnect_user_rvalue = disconnect_backend(username);
    if (disconnect_user_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // delete username from users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "r");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    FILE *temp_users_file = fopen("temp_users.csv", "w");
    if (temp_users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    char line[MAXLINE];
    while (fgets(line, MAXLINE, users_file) != NULL) {
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

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
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
int publish_file(struct soap*, USERNAME username, FILENAME filename, DESCRIPTION description, int *res) {
    // check if user is registered
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        *res = 2;
        return SOAP_OK;
    } else if (check_user_connection_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // check if file has been published by user
    int check_file_existance_rvalue = check_published_file_existance(username, filename);
    if (check_file_existance_rvalue == 1) {
        *res = 3;
        return SOAP_OK;
    } else if (check_file_existance_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
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
        *res = -1;
        return SOAP_OK;
    }

    // write filename;description to file end
    fprintf(username_file, "%s;%s\n", filename, description);
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
}

/**
* @brief adds username, ip and port to connected.csv and creates username file in files folder
* @param client_socket socket of client
* @return 0 if successful
* @return 1 if user doesn't exist
* @return 2 if user is already connected
* @return -1 if error
*/
int connect_user(struct soap*, USERNAME username, IP_ADDRESS ip, PORT port, int *res) {
    // check if user is registered
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // check if user is connected already
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 1) {
        *res = 2;
        return SOAP_OK;
    } else if (check_user_connection_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // write username;ip;port to connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "a");
    if (connected_file == NULL) {
        pthread_mutex_unlock(&connected_file_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }
    fprintf(connected_file, "%s;%s;%s\n", username, ip, port);
    fclose(connected_file);
    pthread_mutex_unlock(&connected_file_lock);

    // create/clear username file in files folder
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "w");
    free(username_filename);
    if (username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
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
int delete_file(struct soap*, USERNAME username, FILENAME filename, int *res) {
    // check if user exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        *res = 2;
        return SOAP_OK;
    } else if (check_user_connection_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // check if the file has been published by the user
    int check_published_file_existance_rvalue = check_published_file_existance(username, filename);
    if (check_published_file_existance_rvalue == 0) {
        *res = 3;
        return SOAP_OK;
    } else if (check_published_file_existance_rvalue < 0) {
        *res = -1;
        return SOAP_OK;
    }

    // remove the file from the users file
    // open the user file
    char *username_filename = malloc(strlen(files_foldername) + strlen(username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "r");
    if (username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    // create temp file to replace with user file
    char *temp_file = malloc(strlen(files_foldername) + strlen(username) + strlen(filename) + 3);
    asprintf(&temp_file, "%s%s_%s", files_foldername, username, filename);
    FILE *temp_username_file = fopen(temp_file, "w");
    if (temp_username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    // go line by line in username file and check if the line contains the filename
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

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
}

/**
* @brief gets all users in connected.csv and sends their info to the client
* @param client_socket socket of client
* @return 0 if successful
* @return -1 if error
*/
int list_users(struct soap*, USERNAME username, int *usernum, struct user user_list[100], int *res) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = 3;
        return SOAP_OK;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        *res = 2;
        return SOAP_OK;
    } else if (check_user_connection_rvalue < 0) {
        *res = 3;
        return SOAP_OK;
    }

    // open connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "r");
    if (connected_file == NULL) {
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    *usernum = 0;
    char line[MAXLINE];

    // get user's info from connected.csv
    while (fgets(line, MAXLINE, connected_file) != 0) {
        strcpy(user_list[*usernum].username, strtok(line, ";"));
        strcpy(user_list[*usernum].ip, strtok(NULL, ";"));
        strcpy(user_list[*usernum].port, strtok(NULL, ";"));
        *usernum += 1;
    }

    fclose(connected_file);
    pthread_mutex_unlock(&connected_file_lock);

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
}

int list_content(struct soap*, USERNAME username, USERNAME requested_username, int *filenum, struct file file_list[500], int *res) {
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *res = 1;
        return SOAP_OK;
    } else if (check_username_existence_rvalue < 0) {
        *res = 0;
        return SOAP_OK;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        *res = 2;
        return SOAP_OK;
    } else if (check_user_connection_rvalue < 0) {
        *res = 3;
        return SOAP_OK;
    }

    // check requested username is connected
    int check_requested_user_connection_rvalue = check_user_connection(requested_username);
    if (check_requested_user_connection_rvalue == 0) {
        *res = 2;
        return SOAP_OK;
    } else if (check_requested_user_connection_rvalue < 0) {
        *res = 3;
        return SOAP_OK;
    }

    // open username file
    char *username_filename = malloc(strlen(files_foldername) + strlen(requested_username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, requested_username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "r");
    free(username_filename);
    if (username_file == NULL) {
        perror("fopen");
        *res = -1;
        return SOAP_OK;
    }

    // get files from username file
    char line[MAXLINE];
    while (fgets(line, MAXLINE, username_file) != 0) {
        if ((strcmp(line, "\n") == 0) || (strcmp(line, "") == 0)) {
            break;
        }
        strcpy(file_list[*filenum].filename, strtok(line, ";"));
        strcpy(file_list[*filenum].description, strtok(NULL, ";"));
        *filenum += 1;
    }

    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    printf("OPERATION FROM %s\n", username);
    *res = 0;
    return SOAP_OK;
}
