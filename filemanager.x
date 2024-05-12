const VERNUM = 1;
const PRINTOPERATIONVER = 1;
const PRINTFILEOPERATIONVER = 2;

const OPERATION_SIZE = 256;
const USERNAME_SIZE = 256;
const DATETIME_SIZE = 20;
const FILENAME_SIZE = 256;

typedef string OPERATION<OPERATION_SIZE>;
typedef string USERNAME<USERNAME_SIZE>;
typedef string DATETIME<DATETIME_SIZE>;
typedef string FILENAME<FILENAME_SIZE>;

program filemanager {
    version VERNUM {
        int print_operation(USERNAME username, OPERATION operation, DATETIME datetime) = PRINTOPERATIONVER;
        int print_file_operation(USERNAME username, OPERATION operation, FILENAME filename, DATETIME datetime) = PRINTFILEOPERATIONVER;
    } = 1;
} = 1;
