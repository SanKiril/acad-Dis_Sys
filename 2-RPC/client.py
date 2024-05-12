from enum import Enum
import argparse
import socket
import threading
import json
import os
import errno
import io
from time import sleep
import signal

# messages size in bytes
EXECUTION_STATUS_SIZE = 1
NUMBER_USERS_SIZE = 4
NUMBER_FILES_SIZE = 4
USERNAME_SIZE = 256
FILENAME_SIZE = 256
DESCRIPTION_SIZE = 256
IP_ADDRESS_SIZE = 16
PORT_SIZE = 6
CLIENT_CONNECTIONS = 1


class client:
    def __init__(self):
        self.__username = ""
        self.__server_socket = None
        self.__server_thread = None
    
    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum):
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1

    # ******************** METHODS *******************
    def register(self, username: str) -> int:
        # INPUT VALIDATION
        if " " in username or len(username) > USERNAME_SIZE:
            print("REGISTER FAIL")
            return client.RC.ERROR

        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("REGISTER\0".encode())  # REGISTER ...
                sleep(0.1)
                client_socket.sendall(f"{username}\0".encode())  # ... <username>

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                
                # CHECK RESPONSE FROM SERVER
                if response == '0':
                    print("REGISTER OK")
                    self.__username = username
                    return client.RC.OK
                elif response == '1':
                    print("USERNAME IN USE")
                    return client.RC.USER_ERROR
                else:
                    print("REGISTER FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("REGISTER FAIL")
            return client.RC.ERROR

    def unregister(self, username: str) -> int:
        # INPUT VALIDATION
        if " " in username or len(username) > USERNAME_SIZE:
            print("UNREGISTER FAIL")
            return client.RC.ERROR

        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("UNREGISTER\0".encode())  # UNREGISTER ...
                sleep(0.1)
                client_socket.sendall(f"{username}\0".encode())  # ... <username>

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                
                # CHECK RESPONSE FROM SERVER
                if response == '0':
                    print("UNREGISTER OK")
                    self.__username = ""
                    return client.RC.OK
                elif response == '1':
                    print("USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                else:
                    print("UNREGISTER FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("UNREGISTER FAIL")
            return client.RC.ERROR
    
    def __share_file(self) -> int:
        try:
            self.__server_socket.listen(CLIENT_CONNECTIONS)  # connections at a time
            while self.__server_socket is not None and self.__server_thread is not None:
                try:
                    client_socket = self.__server_socket.accept()[0]
                    with client_socket:
                        if client_socket.recv(9).decode().rstrip('\0') == "GET_FILE":  # GET_FILE ...
                            filename = client_socket.recv(FILENAME_SIZE).decode().rstrip('\0')  # ... Filename
                            try:
                                # SEND FILE TO CLIENT
                                with open(filename, "rb") as file:
                                    client_socket.sendall("0".encode())  # "0"
                                    while True:
                                        data = file.read(io.DEFAULT_BUFFER_SIZE)
                                        if not data:
                                            break
                                        client_socket.sendall(data)
                                return client.RC.OK
                            except (FileNotFoundError, IOError):
                                client_socket.sendall("1".encode())  # "1"
                                return client.RC.USER_ERROR
                        else:
                            client_socket.sendall("2".encode())  # "2"
                            return client.RC.ERROR
                except socket.error as e:
                    if e.errno == errno.EINTR:  # Interrupted system call
                        self.__server_thread.join()
                        self.__server_thread = None
                        return client.RC.ERROR
                    elif e.errno == errno.EWOULDBLOCK:  # Resource temporarily unavailable
                        continue
                    else:
                        return client.RC.ERROR
        except (socket.error):
            return client.RC.ERROR
    
    def connect(self, username: str) -> int:
        # INPUT VALIDATION
        if " " in username or len(username) > USERNAME_SIZE:
            print("CONNECT FAIL")
            return client.RC.ERROR

        # CLIENT-CLIENT CONNECTION
        if self.__server_socket is None:
            try:
                self.__server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.__server_socket.setblocking(False)  # non-blocking socket

                # FIND FREE PORT
                self.__server_socket.bind(("localhost", 0))  # OS assigns a free port
                port = self.__server_socket.getsockname()[1]

                # CREATE THREAD TO LISTEN FOR REQUESTS
                self.__server_thread = threading.Thread(target=self.__share_file)
                self.__server_thread.start()
            except (socket.error):
                print("CONNECT FAIL")
                return client.RC.ERROR
        else:
            port = self.__server_socket.getsockname()[1]


        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("CONNECT\0".encode())  # CONNECT ...
                sleep(0.1)
                client_socket.sendall(f"{username}\0".encode())  # ... <username> ...
                sleep(0.1)
                client_socket.sendall(f"{port}\0".encode())  # ... Port

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                
                # CHECK RESPONSE
                if response == '0':
                    print("CONNECT OK")
                    return client.RC.OK
                elif response == '1':
                    print("CONNECT FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif response == '2':
                    print("CONNECT FAIL, USER ALREADY CONNECTED")
                    return client.RC.USER_ERROR
                else:
                    print(f"CONNECT FAIL ({response})")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("CONNECT FAIL")
            return client.RC.ERROR

    def disconnect(self, username: str) -> int:
        # INPUT VALIDATION
        if " " in username or len(username) > USERNAME_SIZE:
            print("DISCONNECT FAIL")
            return client.RC.ERROR

        # CLIENT-CLIENT CONNECTION
        if self.__server_socket is not None:
            try:
                self.__server_socket.close()
                self.__server_socket = None
                self.__server_thread.join()
                self.__server_thread = None
            except (socket.error):
                print("DISCONNECT FAIL")
                return client.RC.ERROR

        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("DISCONNECT\0".encode())  # DISCONNECT ...
                sleep(0.1)
                client_socket.sendall(f"{username}\0".encode())  # ... <username>

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                
                # CHECK RESPONSE
                if response == '0':
                    print("DISCONNECT OK")
                    return client.RC.OK
                elif response == '1':
                    print("DISCONNECT FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif response == '2':
                    print("DISCONNECT FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                else:
                    print("DISCONNECT FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("DISCONNECT FAIL")
            return client.RC.ERROR

    def publish(self, filename: str, description: str) -> int:
        # INPUT VALIDATION
        if " " in filename or len(filename) > FILENAME_SIZE:
            print("PUBLISH FAIL")
            return client.RC.ERROR
        if len(description) > DESCRIPTION_SIZE:
            print("PUBLISH FAIL")
            return client.RC.ERROR

        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("PUBLISH\0".encode())  # PUBLISH ...
                sleep(0.1)
                client_socket.sendall(f"{self.__username}\0".encode())  # ... Username ...
                sleep(0.1)
                client_socket.sendall(f"{filename}\0".encode())  # ... <filename> ...
                sleep(0.1)
                client_socket.sendall(f"{description}\0".encode())  # ... <description>

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status

                # CHECK RESPONSE FROM SERVER
                if response == '0':
                    print("PUBLISH OK")
                    return client.RC.OK
                elif response == '1':
                    print("PUBLISH FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif response == '2':
                    print("PUBLISH FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                elif response == '3':
                    print("PUBLISH FAIL, CONTENT ALREADY PUBLISHED")
                    return client.RC.USER_ERROR
                else:
                    print("PUBLISH FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("PUBLISH FAIL")
            return client.RC.ERROR

    def delete(self, filename: str) -> int:
        # INPUT VALIDATION
        if " " in filename or len(filename) > FILENAME_SIZE:
            print("DELETE FAIL")
            return client.RC.ERROR

        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("DELETE\0".encode())  # DELETE ...
                sleep(0.1)
                client_socket.sendall(f"{self.__username}\0".encode())  # ... Username ...
                sleep(0.1)
                client_socket.sendall(f"{filename}\0".encode())  # ... <filename>

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("DELETE FAIL")
                    client_socket.close()
                    return client.RC.ERROR
                
                # CHECK RESPONSE FROM SERVER
                if response == '0':
                    print("DELETE OK")
                    return client.RC.OK
                elif response == '1':
                    print("DELETE FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif response == '2':
                    print("DELETE FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                elif response == '3':
                    print("DELETE FAIL, CONTENT NOT PUBLISHED")
                    return client.RC.USER_ERROR
                else:
                    print("DELETE FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("DELETE FAIL")
            return client.RC.ERROR

    def listusers(self) -> int:
        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("LIST_USERS\0".encode())  # LIST_USERS ...
                sleep(0.1)
                client_socket.sendall(f"{self.__username}\0".encode())  # ... Username

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status

                # CHECK RESPONSE FROM SERVER
                if response == '0':
                    # GET LIST OF USERS
                    users_list = []
                    output = "LIST_USERS OK\n"
                    number_users = int(client_socket.recv(NUMBER_USERS_SIZE).decode())
                    # number_users = int.from_bytes(client_socket.recv(NUMBER_USERS_SIZE), byteorder='big', signed=False)  # Number of users
                    for _ in range(number_users):
                        user_info = {
                            "Username": client_socket.recv(USERNAME_SIZE).decode().rstrip('\0'),  # Username
                            "IP address": client_socket.recv(IP_ADDRESS_SIZE).decode().rstrip('\0'),  # IP address
                            "Port": client_socket.recv(PORT_SIZE).decode().rstrip('\0\n')   # Port
                        }
                        users_list.append(user_info)
                        output += f"{user_info['Username']} {user_info['IP address']} {user_info['Port']}\n"
                    
                    # SAVE LIST OF USERS TO FILE
                    with open(f"listusers-{self.__username}.json", "w") as file:
                        json.dump(users_list, file, indent=4)

                    print(output)
                    return client.RC.OK
                if response == '1':
                    print("LIST_USERS FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif response == '2':
                    print("LIST_USERS FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                else:
                    print("LIST_USERS FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError, ValueError):
            return client.RC.ERROR

    def listcontent(self, username: str) -> int:
        # INPUT VALIDATION
        if " " in username or len(username) > USERNAME_SIZE:
            print("LIST_CONTENT FAIL")
            return client.RC.ERROR

        # CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((client._server, client._port))

                # SEND REQUEST TO SERVER
                client_socket.sendall("LIST_CONTENT\0".encode())  # LIST_CONTENT ...
                sleep(0.1)
                client_socket.sendall(f"{self.__username}\0".encode())  # ... Username ...
                sleep(0.1)
                client_socket.sendall(f"{username}\0".encode())  # ... <username>

                # RECEIVE RESPONSE FROM SERVER
                response = client_socket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status

                # CHECK RESPONSE FROM SERVER
                if response == '0':
                    # GET LIST OF CONTENTS
                    output = "LIST_CONTENT OK\n"
                    number_files = int(client_socket.recv(NUMBER_USERS_SIZE).decode())  # Number of files
                    for _ in range(number_files):
                        file_info = {
                            "Filename": client_socket.recv(FILENAME_SIZE).decode().rstrip('\0\n'),  # Filename
                            "Description": client_socket.recv(DESCRIPTION_SIZE).decode().rstrip('\0\n'),  # Description
                        }
                        output += f"{file_info['Filename']} \"{file_info['Description']}\"\n"

                    print(output)
                    return client.RC.OK
                if response == '1':
                    print("LIST_CONTENT FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif response == '2':
                    print("LIST_CONTENT FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                elif response == '3':
                    print("LIST_CONTENT FAIL, REMOTE USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                else:
                    print("LIST_CONTENT FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError, ValueError):
            print("LIST_CONTENT FAIL")
            return client.RC.ERROR

    def getfile(self, username: str, remote_filename: str, local_filename: str) -> int:
        # INPUT VALIDATION
        if " " in username or len(username) > USERNAME_SIZE:
            print("GET_FILE FAIL")
            return client.RC.ERROR
        if " " in remote_filename or len(remote_filename) > FILENAME_SIZE:
            print("GET_FILE FAIL")
            return client.RC.ERROR
        if " " in local_filename or len(local_filename) > FILENAME_SIZE:
            print("GET_FILE FAIL")
            return client.RC.ERROR
        
        # GET REMOTE USER INFO
        try:
            with open(f"listusers-{self.__username}.json", 'r') as file:
                users_list = json.load(file)
                user_info = None
                for user in users_list:
                    if user["Username"] == username:
                        user_info = user
                        break
                if user_info is None:
                    print("GET_FILE FAIL")
                    return client.RC.ERROR
        except (FileNotFoundError, json.JSONDecodeError):
            print("GET_FILE FAIL")
            return client.RC.ERROR

        # CLIENT-CLIENT CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
                client_socket.connect((user_info["IP address"], user_info["Port"]))

                # SEND REQUEST TO CLIENT
                client_socket.sendall("GET_FILE\0".encode())  # LIST_CONTENT ...
                sleep(0.1)
                client_socket.sendall(f"{remote_filename}\0".encode())  # ... <remote_filename>
                print("send remote filename")

                # RECEIVE RESPONSE FROM CLIENT
                response = client_socket.recv(bufsize=EXECUTION_STATUS_SIZE)  # Execution status
                try:
                    response.decode()
                except:
                    print("lmao")
                print(type(response))
                # CHECK RESPONSE FROM CLIENT
                if response == '0':
                    try:
                        # WRITE TO LOCAL FILE FROM REMOTE FILE
                        with open(local_filename, "wb") as file:
                            while True:
                                data = client_socket.recv(io.DEFAULT_BUFFER_SIZE)  # content from remote file
                                if not data:
                                    break
                                file.write(data)

                        print("GET_FILE OK")
                        return client.RC.OK
                    except (socket.error, IOError):
                        print("GET_FILE FAIL")
                        os.remove(local_filename)  # remove local file
                        return client.RC.ERROR
                if response == '1':
                    print("GET_FILE FAIL, FILE DOES NOT EXIST")
                    return client.RC.USER_ERROR
                else:
                    print("GET_FILE FAIL")
                    return client.RC.ERROR
        except (socket.error, ConnectionRefusedError):
            print("GET_FILE FAIL")
            return client.RC.ERROR

    def quit(self, _signum=None, _frame=None) -> int:
        if self.__server_socket is not None:
            self.__server_socket.close()
            self.__server_socket = None
            self.__server_thread.join()
            self.__server_thread = None
        print("\n+++ FINISHED +++")
        exit(0)

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    def shell(self):
        while (True):
            try:
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER"):
                        if (len(line) == 2):
                            self.register(line[1])
                        else:
                            print("Syntax error. Usage: REGISTER <username>")

                    elif(line[0]=="UNREGISTER"):
                        if (len(line) == 2):
                            self.unregister(line[1])
                        else:
                            print("Syntax error. Usage: UNREGISTER <username>")

                    elif(line[0]=="CONNECT"):
                        if (len(line) == 2):
                            self.connect(line[1])
                        else:
                            print("Syntax error. Usage: CONNECT <username>")
                    
                    elif(line[0]=="PUBLISH"):
                        if (len(line) == 3):
                            #  Remove first 2 words
                            description = ' '.join(line[2:])
                            self.publish(line[1], description)
                        else:
                            print("Syntax error. Usage: PUBLISH <filename> <description>")

                    elif(line[0]=="DELETE"):
                        if (len(line) == 2):
                            self.delete(line[1])
                        else:
                            print("Syntax error. Usage: DELETE <filename>")

                    elif(line[0]=="LIST_USERS"):
                        if (len(line) == 1):
                            self.listusers()
                        else:
                            print("Syntax error. Usage: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT"):
                        if (len(line) == 2):
                            self.listcontent(line[1])
                        else:
                            print("Syntax error. Usage: LIST_CONTENT <username>")

                    elif(line[0]=="DISCONNECT"):
                        if (len(line) == 2):
                            self.disconnect(line[1])
                        else:
                            print("Syntax error. Usage: DISCONNECT <username>")

                    elif(line[0]=="GET_FILE"):
                        if (len(line) == 4):
                            self.getfile(line[1], line[2], line[3])
                        else:
                            print("Syntax error. Usage: GET_FILE <username> <remote_filename> <local_filename>")

                    elif(line[0]=="QUIT"):
                        self.quit()
                        if (len(line) == 1):
                            self.quit()
                            break
                        else:
                            print("Syntax error. Usage: QUIT")
                    else:
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage():
        print("Usage: python3 client.py -s <server> -p <port>")

    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def parseArguments(argv) -> bool:
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")
            return False
        
        client._server = args.s
        client._port = args.p

        return True

    # ******************** MAIN *********************
    def main(self, argv):
        if (not client.parseArguments(argv)):
            client.usage()
            return

        #  Write code here
        signal.signal(signal.SIGINT, self.quit)
        self.shell()
    

if __name__=="__main__":
    client().main([])