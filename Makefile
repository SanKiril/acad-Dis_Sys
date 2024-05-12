RPC = rpcgen

# Define object files
SERVER_OBJECT = server.o

# Define the executable
SERVER = server

SOCKET_SERVER = server
RPC_SERVER = rpc_server

SOURCES.x = filemanager.x

TARGETS_SVC.c = rpc_files/filemanager_svc.c rpc_files/filemanager_xdr.c rpc_server.c
TARGETS_CLNT.c = rpc_files/filemanager_clnt.c rpc_files/filemanager_xdr.c server.c
TARGETS = rpc_files/filemanager.h rpc_files/filemanager_xdr.c rpc_files/filemanager_clnt.c rpc_files/filemanager_svc.c rpc_files/filemanager_client.c rpc_files/filemanager_server.c

OBJECTS_CLNT = $(TARGETS_CLNT.c:%.c=%.o)
OBJECTS_SVC = $(TARGETS_SVC.c:%.c=%.o)

# Compiler flags 
CPPFLAGS += -D_REENTRANT
CFLAGS += -g -D_GNU_SOURCE
LDLIBS += -lnsl -lpthread -I/usr/include/tirpc -ltirpc -I./rpc_files
RPCGENFLAGS = -NM

# Targets 
all:
	@clear
	@make -s $(SOCKET_SERVER)
	@echo "Compiled rpc socket server"
	@make  -s $(RPC_SERVER)
	@echo "Compiled rpc server"

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
