#define OPERATION_SIZE 256
#define EXECUTION_STATUS_SIZE 1
#define NUMBER_USERS_SIZE 4
#define NUMBER_FILES_SIZE 4
#define USERNAME_SIZE 256
#define FILENAME_SIZE 256
#define DESCRIPTION_SIZE 256
#define IP_ADDRESS_SIZE 16
#define PORT_SIZE 6
#define MAXLINE 4096

typedef char USERNAME[256];
typedef char FILENAME[256];
typedef char DESCRIPTION[256];
typedef char IP_ADDRESS[16];
typedef char PORT[6];

int register_user(USERNAME username, int *res);

int disconnect_user(USERNAME username, int *res);

int unregister_user(USERNAME username, int *res);

int publish_file(USERNAME username, FILENAME filename, DESCRIPTION description, int *res);

int connect_user(USERNAME username, IP_ADDRESS ip, PORT port, int *res);

int delete_file(USERNAME username, FILENAME filename, int *res);

typedef struct user{
    USERNAME username;
    IP_ADDRESS ip;
    PORT port;
};

int list_users(USERNAME username, int *usernum, struct user user_list[100], int *res);

typedef struct file{
    FILENAME filename;
    DESCRIPTION description;
};

int list_content(USERNAME username, USERNAME requested_username, int *filenum, struct file file_list[500], int *res);
