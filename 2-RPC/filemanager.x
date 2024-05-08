const VERNUM = 1;
const REGISTERVER = 1;
const DISCONNECTVER = 2;
const UNREGISTERVER = 3;
const PUBLISHVER = 4;
const CONNECTVER = 5;
const DELETEVER = 6;
const LISTUSERSVER = 7;
const LISTCONTENTVER = 8;

const OPERATION_SIZE = 256;
const EXECUTION_STATUS_SIZE = 1;
const NUMBER_USERS_SIZE = 4;
const NUMBER_FILES_SIZE = 4;
const USERNAME_SIZE = 256;
const FILENAME_SIZE = 256;
const DESCRIPTION_SIZE = 256;
const IP_ADDRESS_SIZE = 16;
const PORT_SIZE = 6;
const MAX_USERS = 100;
const MAX_FILES = 100;
const MAXLINE = 4096;

typedef string USERNAME<256>;
typedef string FILENAME<256>;
typedef string DESCRIPTION<256>;
typedef string IP_ADDRESS<16>;
typedef string PORT<6>;

struct user {
    USERNAME username;
    IP_ADDRESS ip;
    PORT port;
};

struct file{
    FILENAME filename;
    DESCRIPTION description;
};

struct list_users_rvalue {
    int execution_status;
    int usernum;
    struct user user_list[MAX_USERS];
};

struct list_content_rvalue {
    int execution_status;
    int filenum;
    struct file file_list[MAX_FILES];
};

program filemanager {
    version VERNUM {
        int register_user(USERNAME username) = REGISTERVER;
        int disconnect_user(USERNAME username) = DISCONNECTVER;
        int unregister_user(USERNAME username) = UNREGISTERVER;
        int publish_file(USERNAME username, FILENAME filename, DESCRIPTION description) = PUBLISHVER;
        int connect_user(USERNAME username, IP_ADDRESS ip, PORT port) = CONNECTVER;
        int delete_file(USERNAME username, FILENAME filename) = DELETEVER;
        struct list_users_rvalue list_users(USERNAME username) = LISTUSERSVER;
        struct list_content_rvalue list_content(USERNAME username, USERNAME requested_username) = LISTCONTENTVER;
    } = 1;
} = 1;
