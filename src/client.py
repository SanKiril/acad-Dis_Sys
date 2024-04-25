from enum import Enum
import argparse
import socket
import threading


class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1

    # ******************** METHODS *******************
    @staticmethod
    def register(userName: str):
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
                    ClientSocket.sendall(f"{userName}".encode())  # ... <userName>
                except socket.error:
                    print("REGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    BUFFER_SIZE = 1  # response size is 1 byte
                    responses = []
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to REGISTER
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <userName>
                except socket.error:
                    print("REGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("REGISTER FAIL")
            return client.RC.ERROR

        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()
        
        # CHECK RESPONSE FROM SERVER
        if responses[0] == '0' and responses[1] == '0':
            print("REGISTER OK")
            return client.RC.OK
        if responses[0] == '0' and responses[1] == '1':
            print("USERNAME IN USE")
            return client.RC.USER_ERROR
        else:
            print("REGISTER FAIL")
            return client.RC.ERROR

    @staticmethod
    def unregister(userName: str):
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
                    ClientSocket.sendall(f"{userName}".encode())  # ... <userName>
                except socket.error:
                    print("UNREGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    BUFFER_SIZE = 1  # response size is 1 byte
                    responses = []
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to UNREGISTER
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <userName>
                except socket.error:
                    print("UNREGISTER FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("UNREGISTER FAIL")
            return client.RC.ERROR

        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()
        
        # CHECK RESPONSE FROM SERVER
        if responses[0] == '0' and responses[1] == '0':
            print("UNREGISTER OK")
            return client.RC.OK
        if responses[0] == '0' and responses[1] == '1':
            print("USER DOES NOT EXIST")
            return client.RC.USER_ERROR
        else:
            print("UNREGISTER FAIL")
            return client.RC.ERROR
    
    @staticmethod
    def connect(userName: str):
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
                    ClientSocket.sendall(f"{userName}".encode())  # ... <userName> ...
                    ClientSocket.sendall(f"{port}".encode())  # ... <port>
                except socket.error:
                    print("CONNECT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    BUFFER_SIZE = 1  # response size is 1 byte
                    responses = []
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to CONNECT
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <userName>
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <port>
                except socket.error:
                    print("CONNECT FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("CONNECT FAIL")
            return client.RC.ERROR

        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()
        
        # CHECK RESPONSE
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '0':
            print("CONNECT OK")
            return client.RC.OK
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '1':
            print("CONNECT FAIL, USER DOES NOT EXIST")
            return client.RC.USER_ERROR
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '2':
            print("USER ALREADY CONNECTED")
            return client.RC.USER_ERROR
        else:
            print("CONNECT FAIL")
            return client.RC.ERROR

    @staticmethod
    def disconnect(user):
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def publish(userName: str, fileName: str, description: str):
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
                    ClientSocket.sendall(f"{userName}".encode())  # ... <userName> ...
                    ClientSocket.sendall(f"{fileName}".encode())  # ... <fileName> ...
                    ClientSocket.sendall(f"{description}".encode())  # ... <description>
                except socket.error:
                    print("PUBLISH FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR

                # RECEIVE RESPONSE FROM SERVER
                try:
                    BUFFER_SIZE = 1  # response size is 1 byte
                    responses = []
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to PUBLISH
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <userName>
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <fileName>
                    responses.append(ClientSocket.recv(BUFFER_SIZE).decode())  # response to <description>
                except socket.error:
                    print("PUBLISH FAIL")
                    ClientSocket.close()
                    return client.RC.ERROR
        except socket.error:
            print("PUBLISH FAIL")
            return client.RC.ERROR

        # END CLIENT-SERVER CONNECTION
        ClientSocket.close()
        
        # CHECK RESPONSE FROM SERVER
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '0' and responses[3] == '0':
            print("PUBLISH OK")
            return client.RC.OK
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '0' and responses[3] == '1':
            print("PUBLISH FAIL, USER DOES NOT EXIST")
            return client.RC.USER_ERROR
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '0' and responses[3] == '2':
            print("PUBLISH FAIL, USER NOT CONNECTED")
            return client.RC.USER_ERROR
        if responses[0] == '0' and responses[1] == '0' and responses[2] == '0' and responses[3] == '3':
            print("PUBLISH FAIL, CONTENT ALREADY PUBLISHED")
            return client.RC.USER_ERROR
        else:
            print("PUBLISH FAIL")
            return client.RC.ERROR

    @staticmethod
    def delete(fileName):
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def listusers():
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def listcontent(user):
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def getfile(user,  remote_FileName,  local_FileName):
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
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER"):
                        if (len(line) == 2):
                            client.unregister(line[1])
                        else:
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT"):
                        if (len(line) == 2):
                            client.connect(line[1])
                        else:
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH"):
                        if (len(line) == 4):
                            #  Remove first 3 words
                            description = ' '.join(line[3:])
                            client.publish(line[1], line[2], description)
                        else:
                            print("Syntax error. Usage: PUBLISH <userName> <fileName> <description>")

                    elif(line[0]=="DELETE"):
                        if (len(line) == 2):
                            client.delete(line[1])
                        else:
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS"):
                        if (len(line) == 1):
                            client.listusers()
                        else:
                            print("Syntax error. Usage: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT"):
                        if (len(line) == 2):
                            client.listcontent(line[1])
                        else:
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT"):
                        if (len(line) == 2):
                            client.disconnect(line[1])
                        else:
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE"):
                        if (len(line) == 4):
                            client.getfile(line[1], line[2], line[3])
                        else:
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

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
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")

    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def parseArguments(argv) :
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
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])