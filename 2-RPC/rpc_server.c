#include "filemanager.h"
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
* @brief check if username exists in users.csv
* @param username username to check
* @return 1 if exists, 0 otherwise
* @return -1 if error
*/
static int check_username_existence(USERNAME username) {
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
static int check_user_connection(USERNAME username) {
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
bool_t
register_user_1_svc(USERNAME username, int *result,  struct svc_req *rqstp)
{
    // check if username exists in users.csv
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 1) {
        *result = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // open users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "a");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        *result = -1;
        return TRUE;
    }

    // write username to users.csv
    fprintf(users_file, "%s\n", username);
    fclose(users_file);
    pthread_mutex_unlock(&users_file_lock);

    printf("OPERATION FROM %s\n", username);
    *result = 0;
    return TRUE;
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
bool_t
disconnect_user_1_svc(USERNAME username, int *result,  struct svc_req *rqstp)
{
	// check if user exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *result = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // attempt to disconnect user
    int disconnect_backend_rvalue = disconnect_backend(username);
    if (disconnect_backend_rvalue != 0) {
        *result = disconnect_backend_rvalue;
        return TRUE;
    } 
    printf("OPERATION FROM %s\n", username);
    *result = 0;
    return TRUE;
}

/**
* @brief unregister user, deleting it from users.csv file and disconnecting them if they are connected
* @param username username to unregister
* @return 0 if successful
* @return 1 if username doesn't exist
* @return -1 if error
*/
bool_t
unregister_user_1_svc(USERNAME username, int *result,  struct svc_req *rqstp)
{
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *result = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // disconnect user if they are connected
    int disconnect_user_rvalue = disconnect_backend(username);
    if (disconnect_user_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // delete username from users.csv
    pthread_mutex_lock(&users_file_lock);
    FILE *users_file = fopen(users_filename, "r");
    if (users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        *result = -1;
        return TRUE;
    }

    FILE *temp_users_file = fopen("temp_users.csv", "w");
    if (temp_users_file == NULL) {
        pthread_mutex_unlock(&users_file_lock);
        perror("fopen");
        *result = -1;
        return TRUE;
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
    *result = 0;
    return TRUE;
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
bool_t
publish_file_1_svc(USERNAME username, FILENAME filename, DESCRIPTION description, int *result,  struct svc_req *rqstp)
{
    // check if user is registered
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *result = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        *result = 2;
        return TRUE;
    } else if (check_user_connection_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // check if file has been published by user
    int check_file_existance_rvalue = check_published_file_existance(username, filename);
    if (check_file_existance_rvalue == 1) {
        *result = 3;
        return TRUE;
    } else if (check_file_existance_rvalue < 0) {
        *result = -1;
        return TRUE;
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
        *result = -1;
        return TRUE;
    }

    // write filename;description to file end
    fprintf(username_file, "%s;%s\n", filename, description);
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    printf("OPERATION FROM %s\n", username);
    *result = 0;
    return TRUE;
}

/**
* @brief adds username, ip and port to connected.csv and creates username file in files folder
* @param client_socket socket of client
* @return 0 if successful
* @return 1 if user doesn't exist
* @return 2 if user is already connected
* @return -1 if error
*/
bool_t
connect_user_1_svc(USERNAME username, IP_ADDRESS ip, PORT port, int *result,  struct svc_req *rqstp)
{
    // check if user is registered
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *result = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // check if user is connected already
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 1) {
        *result = 2;
        return TRUE;
    } else if (check_user_connection_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // write username;ip;port to connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "a");
    if (connected_file == NULL) {
        pthread_mutex_unlock(&connected_file_lock);
        perror("fopen");
        *result = -1;
        return TRUE;
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
        *result = -1;
        return TRUE;
    }
    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    printf("OPERATION FROM %s\n", username);
    *result = 0;
    return TRUE;
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
bool_t
delete_file_1_svc(USERNAME username, FILENAME filename, int *result,  struct svc_req *rqstp)
{
    // check if user exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        *result = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        *result = 2;
        return TRUE;
    } else if (check_user_connection_rvalue < 0) {
        *result = -1;
        return TRUE;
    }

    // check if the file has been published by the user
    int check_published_file_existance_rvalue = check_published_file_existance(username, filename);
    if (check_published_file_existance_rvalue == 0) {
        *result = 3;
        return TRUE;
    } else if (check_published_file_existance_rvalue < 0) {
        *result = -1;
        return TRUE;
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
        *result = -1;
        return TRUE;
    }

    // create temp file to replace with user file
    char *temp_file = malloc(strlen(files_foldername) + strlen(username) + strlen(filename) + 3);
    asprintf(&temp_file, "%s%s_%s", files_foldername, username, filename);
    FILE *temp_username_file = fopen(temp_file, "w");
    if (temp_username_file == NULL) {
        pthread_mutex_unlock(&files_folder_lock);
        perror("fopen");
        *result = -1;
        return TRUE;
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
    *result = 0;
    return TRUE;
}

bool_t
list_users_1_svc(USERNAME username, struct list_users_rvalue *result,  struct svc_req *rqstp)
{
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        result->execution_status = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        result->execution_status = 3;
        return TRUE;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        result->execution_status = 2;
        return TRUE;
    } else if (check_user_connection_rvalue < 0) {
        result->execution_status = 3;
        return TRUE;
    }

    // open connected.csv
    pthread_mutex_lock(&connected_file_lock);
    FILE *connected_file = fopen(connected_filename, "r");
    if (connected_file == NULL) {
        perror("fopen");
        result->execution_status = -1;
        return TRUE;
    }

    int usernum = 0;
    char line[MAXLINE];

    // get user's info from connected.csv
    while (fgets(line, MAXLINE, connected_file) != 0) {
        strcpy(result->user_list[usernum].username, strtok(line, ";"));
        strcpy(result->user_list[usernum].ip, strtok(NULL, ";"));
        strcpy(result->user_list[usernum].port, strtok(NULL, ";"));
        usernum += 1;
    }

	result->usernum = usernum;

    fclose(connected_file);
    pthread_mutex_unlock(&connected_file_lock);

    printf("OPERATION FROM %s\n", username);
    result->execution_status = 0;
    return TRUE;
}

bool_t
list_content_1_svc(USERNAME username, USERNAME requested_username, struct list_content_rvalue *result,  struct svc_req *rqstp)
{
    // check if username exists
    int check_username_existence_rvalue = check_username_existence(username);
    if (check_username_existence_rvalue == 0) {
        result->execution_status = 1;
        return TRUE;
    } else if (check_username_existence_rvalue < 0) {
        result->execution_status = 0;
        return TRUE;
    }

    // check if user is connected
    int check_user_connection_rvalue = check_user_connection(username);
    if (check_user_connection_rvalue == 0) {
        result->execution_status = 2;
        return TRUE;
    } else if (check_user_connection_rvalue < 0) {
        result->execution_status = 3;
        return TRUE;
    }

    // check requested username is connected
    int check_requested_user_connection_rvalue = check_user_connection(requested_username);
    if (check_requested_user_connection_rvalue == 0) {
        result->execution_status = 2;
        return TRUE;
    } else if (check_requested_user_connection_rvalue < 0) {
        result->execution_status = 3;
        return TRUE;
    }

    // open username file
    char *username_filename = malloc(strlen(files_foldername) + strlen(requested_username) + 2);
    asprintf(&username_filename, "%s%s", files_foldername, requested_username);
    pthread_mutex_lock(&files_folder_lock);
    FILE *username_file = fopen(username_filename, "r");
    free(username_filename);
    if (username_file == NULL) {
        perror("fopen");
        result->execution_status = -1;
        return TRUE;
    }

	int filenum;
    // get files from username file
    char line[MAXLINE];
    while (fgets(line, MAXLINE, username_file) != 0) {
        if ((strcmp(line, "\n") == 0) || (strcmp(line, "") == 0)) {
            break;
        }
        strcpy(result->file_list[filenum].filename, strtok(line, ";"));
        strcpy(result->file_list[filenum].description, strtok(NULL, ";"));
        filenum += 1;
    }
	result->filenum = filenum; 

    fclose(username_file);
    pthread_mutex_unlock(&files_folder_lock);

    printf("OPERATION FROM %s\n", username);
    result->execution_status = 0;
    return TRUE;
}

int
filemanager_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	// xdr_free (xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}
