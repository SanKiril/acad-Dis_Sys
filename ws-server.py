import sys
from datetime import datetime
from spyne import Application, ServiceBase, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
from wsgiref.simple_server import make_server


WS_PORT = 8000

class DateTimeService(ServiceBase):
    @rpc(_returns=str)
    def datetime(self):
        return datetime.now().strftime("%d/%m/%Y %H:%M:%S")


if __name__ == "__main__":
    application = Application(services=[DateTimeService],
                            tns='http://tests.python-zeep.org/',
                            in_protocol=Soap11(validator='lxml'),
                            out_protocol=Soap11())
    application = WsgiApplication(application)
    server = make_server("localhost", WS_PORT, application)
    server.serve_forever()