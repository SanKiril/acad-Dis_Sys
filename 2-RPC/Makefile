CC = gcc

# Compiler flags
CLFAGS = -Wall -g -Wno-unused-variable
LDFLAGS = -lrt -pthread -lnsl -lpthread -ltirpc -I/usr/include/tirpc
RPCGENFLAGS = -NM

# Server files
SERVER = server
SERVER.c = server.c

# RPC files
RPC_SERVER = rpc_server
RPCGEN = rpcgen
SOURCES.x = filemanager.x
TARGETS.c = filemanager_svc.c rpc_server.c filemanager_xdr.c filemanager_clnt.c
TARGETS.h = filemanager.h
TARGETS = $(TARGETS.h) + $(TARGETS.c)
RPCGEN_FILES = filemanager_xrd.c filemanager_svc.c filemanager_clnt.c
OBJECTS = $(TARGETS.c:%.c=%.o)
RPC_SERVER_OBJECTS = filemanager_svc.o rpc_server.o filemanager_xdr.o
RPC_CLIENT_OBJECTS = filemanager_clnt.o

all:
	@clear
	@make -s clean
	@make -s $(SERVER)

$(OBJECTS): $(TARGETS.c)
$(RPCGEN): $(RPCGEN_FILES)

$(RPCGEN_FILES) $(TARGETS.h): $(SOURCES.x)
	$(RPCGEN) $(RPCGENFLAGS) $(SOURCES.x)
	$(RPCGEN) $(RPCGENFLAGS) -s tcp $(SOURCES.x) > filemanager_svc.c 

$(RPC_SERVER): $(RPC_SERVER_OBJECTS) $(RPCGEN_FILES)
	$(LINK.c) -o $(RPC_SERVER) $(RPC_SERVER_OBJECTS)
	echo "Compiled RPC server"

$(SERVER): $(SERVER.c) $(RPC_SERVER)
	$(LINK.c) -o server filemanager_clnt.c server.c
	echo "Compiled server"

clean:
	@rm -f filemanager.h
	@rm -f filemanager_*
	@rm -f files/*
	@rm -f *.csv
	@rm -f *.json
	@rm -f $(OBJECTS) $(SERVER_OBJECT) $(RPC_SERVER) $(SERVER)
	@echo "Cleaned up"
