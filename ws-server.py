import sys
from datetime import datetime
from spyne import Application, ServiceBase, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
from wsgiref.simple_server import make_server


class DateTimeService(ServiceBase):
    @rpc(_returns=str)
    def datetime(self):
        return datetime.now().strftime("%d/%m/%Y %H:%M:%S")


def main():
    # INPUT VALIDATION
    if len(sys.argv) != 3 or sys.argv[1] != "-p" or not sys.argv[2].isdigit():
        print("Usage: python3 ws-server.py -p <port>")
        return
    if int(sys.argv[2]) < 1024 or int(sys.argv[2]) > 65535:
        print("Error: Port must be in the range 1024 <= port <= 65535")
        return
    
    application = Application(services=[DateTimeService],
                            tns='http://tests.python-zeep.org/',
                            in_protocol=Soap11(validator='lxml'),
                            out_protocol=Soap11())
    application = WsgiApplication(application)
    server = make_server("localhost", int(sys.argv[2]), application)
    server.serve_forever()


if __name__ == "__main__":
    main()
