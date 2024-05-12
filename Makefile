RPC = rpcgen

# Define object files
SERVER_OBJECT = server.o

# Define the executable
SERVER = executables/server

SOCKET_SERVER = executables/rpc_socket_server
RPC_SERVER = executables/rpc_server

SOURCES.x = filemanager.x

TARGETS_SVC.c = rpc_files/filemanager_svc.c rpc_files/filemanager_xdr.c rpc_server.c
TARGETS_CLNT.c = rpc_files/filemanager_clnt.c rpc_files/filemanager_xdr.c rpc_socket_server.c
TARGETS = rpc_files/filemanager.h rpc_files/filemanager_xdr.c rpc_files/filemanager_clnt.c rpc_files/filemanager_svc.c rpc_files/filemanager_client.c rpc_files/filemanager_server.c

OBJECTS_CLNT = $(TARGETS_CLNT.c:%.c=%.o)
OBJECTS_SVC = $(TARGETS_SVC.c:%.c=%.o)

# Compiler flags 
CPPFLAGS += -D_REENTRANT
CFLAGS += -g 
LDLIBS += -lnsl -lpthread -I/usr/include/tirpc -ltirpc -I./rpc_files
RPCGENFLAGS = -NM

# Targets 
all:
	@clear
	@make -s socket
	@make -s clean
	@make -s rpc

rpc:
	@make -s clean
	@make -s $(SOCKET_SERVER)
	@echo "Compiled rpc socket server"
	@make  -s $(RPC_SERVER)
	@echo "Compiled rpc server"

socket:
	@make -s clean
	@make -s $(SERVER)
	@echo "Compiled socket server"

clean:
	 @$(RM) core $(TARGETS) $(OBJECTS_CLNT) $(OBJECTS_SVC) $(SOCKET_SERVER) $(RPC_SERVER)
	 @$(RM) $(SERVER_OBJECT) $(SERVER)
	 @$(RM) -f Makefile.*

$(RPC): $(TARGETS)
$(TARGETS) : $(SOURCES.x) 
	rpcgen $(RPCGENFLAGS) filemanager.x
	mv filemanager_* rpc_files
	mv filemanager.h rpc_files

$(OBJECTS_CLNT) :  %.o: %.c
	$(LINK.c) $(LDLIBS) -c $< -o $@

$(filter-out rpc_files/filemanager_xdr.o,$(OBJECTS_SVC)): %.o: %.c
	$(LINK.c) $(LDLIBS) -c $< -o $@

$(SOCKET_SERVER) : $(OBJECTS_CLNT) 
	$(LINK.c) -o $(SOCKET_SERVER) $(OBJECTS_CLNT) $(LDLIBS) 

$(RPC_SERVER) : $(OBJECTS_SVC) 
	$(LINK.c) -o $(RPC_SERVER) $(OBJECTS_SVC) $(LDLIBS)

$(SERVER): $(SERVER_OBJECT)
	$(LINK.c) $(CFLAGS) -o $@ $^

$(SERVER_OBJECT): server.c
	$(LINK.c) $(CFLAGS) -c $< -o $@
