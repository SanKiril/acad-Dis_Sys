from enum import Enum
import argparse
import socket

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
    def register(user: str):
        # START CONNECTION
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                try:
                    s.connect((client._server, client._port))

                    # SEND REQUEST
                    s.sendall("REGISTER".encode())  # REGISTER ...
                    s.sendall(f"{user}".encode())  # ... user

                    # RECEIVE RESPONSE
                    try:
                        responses = []
                        responses.append(s.recv(1).decode())  # 1 byte buffer
                        responses.append(s.recv(1).decode())  # 1 byte buffer
                    except socket.error:
                        print("REGISTER FAIL")
                        return client.RC.ERROR
                except ConnectionRefusedError:
                    print("REGISTER FAIL")
                    return client.RC.ERROR
        except socket.error:
            print("REGISTER FAIL")
            return client.RC.ERROR
        
        # END CONNECTION
        try:
            s.close()
        except UnboundLocalError:
            print("REGISTER FAIL")
            return client.RC.ERROR
        
        # CHECK RESPONSE
        if all(response == '1' for response in responses):
            print("USERNAME IN USE")
            return client.RC.USER_ERROR
        elif all(response == '2' for response in responses):
            print("REGISTER FAIL")
            return client.RC.ERROR
        else:
            print("REGISTER OK")
            return client.RC.OK

    @staticmethod
    def unregister(user: str):
        # SEND REQUEST
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR
    
    @staticmethod
    def connect(user):
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def disconnect(user):
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
        return client.RC.ERROR

    @staticmethod
    def publish(fileName,  description):
        # SEND REQUEST 
        # RECEIVE RESPONSE
        # CLOSE CONNECTION
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
        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
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