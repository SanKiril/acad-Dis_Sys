CC = gcc
SOAPCPP = soapcpp2
CFLAGS = -Wall -Wextra -g
LDFLAGS = -I gsoap
SOAPFLAGS = -c -b

NULL_FILE = /dev/null

SOAPC_OBJECT = soapC.o
SOAPC_FILE = soapC.c

SOAP_SERVER_OBJECT = soapServer.o
SOAP_SERVER_FILE = soapServer.c

SERVER_OBJECT = filemanagerServer.o
SERVER_FILE = filemanagerServer.c
SERVER = server


all:
	@clear
	@make -s clean
	@make -s $(SERVER)
	@echo "Compiled server"

$(SERVER): $(SOAPC_OBJECT) $(SERVER_OBJECT) $(SOAP_SERVER_OBJECT)
	$(CC) $(CFLAGS) -o $@ $^ -lgsoap

soapc:$(SOAPC_OBJECT)
$(SOAPC_OBJECT): $(SOAPC_FILE)
	$(CC) $(CFLAGS) $(LDFLAGS) -c $^ -o $@

serverobject:$(SERVER_OBJECT)
$(SERVER_OBJECT): $(SERVER_FILE)
	$(CC) $(CFLAGS) -c $^ -o $@

soapserver:$(SOAP_SERVER_OBJECT)
$(SOAP_SERVER_OBJECT): $(SOAP_SERVER_FILE)
	$(CC) $(CFLAGS) -c $^ -o $@

soap:$(SOAPC_FILE)
$(SOAPC_FILE) $(SOAP_SERVER_FILE): filemanager.h
	$(SOAPCPP) $(SOAPFLAGS) $^ &>> $(NULL_FILE)


clean:
	@rm -f files/*
	@rm -f *.csv
	@rm -f *.json
	@rm -f *.o
	@rm -f soap*
	@echo "Cleaned up"
