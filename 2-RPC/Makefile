CC = gcc

# Compiler flags
CLFAGS = -Wall -g -Wno-unused-variable
LDFLAGS = -lrt -pthread -lnsl -lpthread -ltirpc
RPCGENFLAGS = -NM

# Server files
SERVER = server
SERVER_OBJECT = server.o
SERVER.c = server.c

# RPC files
RPC_SERVER = rpc_server
RPCGEN = rpcgen
SOURCES.x = filemanager.x
TARGETS.c = filemanager_svc.c rpc_server.c filemanager_xdr.c
TARGETS = filemanager.h + $(TARGETS.c)
RPCGEN_FILES = filemanager_xrd.c filemanager_svc.c filemanager_clnt.c
OBJECTS = $(TARGETS.c:%.c=%.o)

all:
	@make -s clean
	@make -s $(SERVER)
	@make -s $(RPC_SERVER)

$(OBJECTS): $(TARGETS.c)
$(RPCGEN): $(RPCGEN_FILES)

$(RPCGEN_FILES): $(SOURCES.x)
	$(RPCGEN) $(RPCGENFLAGS) $(SOURCES.x)
	$(RPCGEN) $(RPCGENFLAGS) -s tcp $(SOURCES.x) > filemanager_svc.c 
	echo "Generated RPC files"

$(RPC_SERVER): $(OBJECTS) $(RPCGEN_FILES)
	$(LINK.c) -o $(RPC_SERVER) $(OBJECTS)
	echo "Compiled RPC server"

$(SERVER): $(SERVER_OBJECT)
	$(CC) $(CFLAGS) -o $@ $^
	echo "Compiled server"

$(SERVER_OBJECT): $(SERVER.c)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f filemanager.h
	@rm -f filemanager_*
	@rm -f files/*
	@rm -f *.csv
	@rm -f *.json
	@rm -f $(OBJECTS) $(SERVER_OBJECT) $(RPC_SERVER) $(SERVER)
	@echo "Cleaned up"
