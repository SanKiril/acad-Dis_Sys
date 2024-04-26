from enum import Enum
import argparse
import socket
import threading


# messages size in bytes
EXECUTION_STATUS_SIZE = 1
NUMBER_USERS_SIZE = 4
NUMBER_FILES_SIZE = 4
USERNAME_SIZE = 256
FILENAME_SIZE = 256
DESCRIPTION_SIZE = 256
IP_ADDRESS_SIZE = 16
PORT_SIZE = 6


class client:

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
    @staticmethod
    def register(username: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("REGISTER FAIL")
            return client.RC.ERROR

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("REGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("REGISTER".encode())  # REGISTER ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username>
                except socket.error:
                    print("REGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("REGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("REGISTER FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE FROM SERVER
        if response == '0':
            print("REGISTER OK")
            retval = client.RC.OK
        elif response == '1':
            print("USERNAME IN USE")
            retval = client.RC.USER_ERROR
        else:
            print("REGISTER FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()
        
        return retval

    @staticmethod
    def unregister(username: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("UNREGISTER FAIL")
            return client.RC.ERROR

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("UNREGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("UNREGISTER".encode())  # UNREGISTER ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username>
                except socket.error:
                    print("UNREGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("UNREGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("UNREGISTER FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE FROM SERVER
        if response == '0':
            print("UNREGISTER OK")
            retval = client.RC.OK
        elif response == '1':
            print("USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        else:
            print("UNREGISTER FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()

        return retval
    
    @staticmethod
    def connect(username: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("CONNECT FAIL")
            return client.RC.ERROR

        # SEARCH FREE VALID PORT

        ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("CONNECT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("CONNECT".encode())  # CONNECT ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username> ...
                    ClientSocket.sendall(f"{port}".encode())  # ... <port>
                except socket.error:
                    print("CONNECT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("CONNECT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("CONNECT FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE
        if response == '0':
            print("CONNECT OK")
            retval = client.RC.OK
        elif response == '1':
            print("CONNECT FAIL, USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        elif response == '2':
            print("USER ALREADY CONNECTED")
            retval = client.RC.USER_ERROR
        else:
            print("CONNECT FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()

        return retval

    @staticmethod
    def disconnect(user) -> int:
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def publish(username: str, filename: str, description: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("PUBLISH FAIL")
            return client.RC.ERROR
        if " " in filename or len(filename) > FILENAME_SIZE:
            print("PUBLISH FAIL")
            return client.RC.ERROR
        if len(description) > DESCRIPTION_SIZE:
            print("PUBLISH FAIL")
            return client.RC.ERROR

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("PUBLISH FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("PUBLISH".encode())  # PUBLISH ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username> ...
                    ClientSocket.sendall(f"{filename}".encode())  # ... <filename> ...
                    ClientSocket.sendall(f"{description}".encode())  # ... <description>
                except socket.error:
                    print("PUBLISH FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("PUBLISH FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("PUBLISH FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE FROM SERVER
        if response == '0':
            print("PUBLISH OK")
            retval = client.RC.OK
        elif response == '1':
            print("PUBLISH FAIL, USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        elif response == '2':
            print("PUBLISH FAIL, USER NOT CONNECTED")
            retval = client.RC.USER_ERROR
        elif response == '3':
            print("PUBLISH FAIL, CONTENT ALREADY PUBLISHED")
            retval = client.RC.USER_ERROR
        else:
            print("PUBLISH FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()

        return retval

    @staticmethod
    def delete(username: str, filename: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("DELETE FAIL")
            return client.RC.ERROR
        if " " in filename or len(filename) > FILENAME_SIZE:
            print("DELETE FAIL")
            return client.RC.ERROR

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("DELETE FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("DELETE".encode())  # DELETE ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username> ...
                    ClientSocket.sendall(f"{filename}".encode())  # ... <filename>
                except socket.error:
                    print("DELETE FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("DELETE FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("DELETE FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE FROM SERVER
        if response == '0':
            print("DELETE OK")
            retval = client.RC.OK
        elif response == '1':
            print("DELETE FAIL, USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        elif response == '2':
            print("DELETE FAIL, USER NOT CONNECTED")
            retval = client.RC.USER_ERROR
        elif response == '3':
            print("DELETE FAIL, CONTENT NOT PUBLISHED")
            retval = client.RC.USER_ERROR
        else:
            print("DELETE FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()

        return retval

    @staticmethod
    def listusers(username: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("LIST_USERS FAIL")
            return client.RC.ERROR

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("LIST_USERS FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("LIST_USERS".encode())  # LIST_USERS ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username>
                except socket.error:
                    print("LIST_USERS FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("LIST_USERS FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("LIST_USERS FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE FROM SERVER
        if response == '0':
            output = "LIST_USERS OK\n"
            try:
                number_users = int(ClientSocket.recv(NUMBER_USERS_SIZE).decode())    # Number of users

                for _ in range(number_users):
                    user_info = [
                        ClientSocket.recv(USERNAME_SIZE).decode(),  # Username
                        ClientSocket.recv(IP_ADDRESS_SIZE).decode(),  # IP address
                        ClientSocket.recv(PORT_SIZE).decode()   # Port
                    ]
                    output += f"{user_info[0]} {user_info[1]} {user_info[2]}\n"

                print(output)
                retval = client.RC.OK
            except (socket.error, ValueError):
                print("LIST_USERS FAIL")
                ClientSocket.close()
                return client.RC.ERROR
        if response == '1':
            print("LIST_USERS FAIL, USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        elif response == '2':
            print("LIST_USERS FAIL, USER NOT CONNECTED")
            retval = client.RC.USER_ERROR
        else:
            print("LIST_USERS FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()

        return retval

    @staticmethod
    def listcontent(username: str, remote_username: str) -> int:
        # INPUT VALIDATION
        if len(username) > USERNAME_SIZE:
            print("LIST_CONTENT FAIL")
            return client.RC.ERROR
        if len(remote_username) > USERNAME_SIZE:
            print("LIST_CONTENT FAIL")
            return client.RC.ERROR

        # START CLIENT-SERVER CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ClientSocket:
                try:
                    ClientSocket.connect((client._server, client._port))
                except ConnectionRefusedError:
                    print("LIST_CONTENT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # SEND REQUEST TO SERVER
                try:
                    ClientSocket.sendall("LIST_CONTENT".encode())  # LIST_CONTENT ...
                    ClientSocket.sendall(f"{username}".encode())  # ... <username> ...
                    ClientSocket.sendall(f"{remote_username}".encode())  # ... <remote_username>
                except socket.error:
                    print("LIST_CONTENT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    response = ClientSocket.recv(EXECUTION_STATUS_SIZE).decode()  # Execution status
                except socket.error:
                    print("LIST_CONTENT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("LIST_CONTENT FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE FROM SERVER
        if response == '0':
            output = "LIST_CONTENT OK\n"
            try:
                number_files = int(ClientSocket.recv(NUMBER_FILES_SIZE).decode())    # Number of files

                for _ in range(number_files):
                    file_info = [
                        ClientSocket.recv(FILENAME_SIZE).decode(),  # Filename
                        ClientSocket.recv(DESCRIPTION_SIZE).decode(),  # Description
                    ]
                    output += f"{file_info[0]} \"{file_info[1]}\"\n"

                print(output)
                retval = client.RC.OK
            except (socket.error, ValueError):
                print("LIST_CONTENT FAIL")
                ClientSocket.close()
                return client.RC.ERROR
        if response == '1':
            print("LIST_CONTENT FAIL, USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        elif response == '2':
            print("LIST_CONTENT FAIL, USER NOT CONNECTED")
            retval = client.RC.USER_ERROR
        elif response == '3':
            print("LIST_CONTENT FAIL, REMOTE USER DOES NOT EXIST")
            retval = client.RC.USER_ERROR
        else:
            print("LIST_CONTENT FAIL")
            retval = client.RC.ERROR
        
        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()

        return retval

    @staticmethod
    def getfile(username: str, remote_filename: str, local_filename: str) -> int:
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():
        while (True):
            try:
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER"):
                        if (len(line) == 2):
                            client.register(line[1])
                        else:
                            print("Syntax error. Usage: REGISTER <username>")

                    elif(line[0]=="UNREGISTER"):
                        if (len(line) == 2):
                            client.unregister(line[1])
                        else:
                            print("Syntax error. Usage: UNREGISTER <username>")

                    elif(line[0]=="CONNECT"):
                        if (len(line) == 2):
                            client.connect(line[1])
                        else:
                            print("Syntax error. Usage: CONNECT <username>")
                    
                    elif(line[0]=="PUBLISH"):
                        if (len(line) == 4):
                            #  Remove first 3 words
                            description = ' '.join(line[3:])
                            client.publish(line[1], line[2], description)
                        else:
                            print("Syntax error. Usage: PUBLISH <username> <filename> <description>")

                    elif(line[0]=="DELETE"):
                        if (len(line) == 3):
                            client.delete(line[1], line[2])
                        else:
                            print("Syntax error. Usage: DELETE <username> <filename>")

                    elif(line[0]=="LIST_USERS"):
                        if (len(line) == 2):
                            client.listusers(line[1])
                        else:
                            print("Syntax error. Usage: LIST_USERS <username>")

                    elif(line[0]=="LIST_CONTENT"):
                        if (len(line) == 3):
                            client.listcontent(line[1], line[2])
                        else:
                            print("Syntax error. Usage: LIST_CONTENT <username> <remote_username>")

                    elif(line[0]=="DISCONNECT"):
                        if (len(line) == 2):
                            client.disconnect(line[1])
                        else:
                            print("Syntax error. Usage: DISCONNECT <username>")

                    elif(line[0]=="GET_FILE"):
                        if (len(line) == 4):
                            client.getfile(line[1], line[2], line[3])
                        else:
                            print("Syntax error. Usage: GET_FILE <username> <remote_filename> <local_filename>")

                    elif(line[0]=="QUIT"):
                        if (len(line) == 1):
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
    @staticmethod
    def main(argv):
        if (not client.parseArguments(argv)):
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])