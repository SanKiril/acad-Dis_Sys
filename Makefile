RPC = rpcgen

# Define object files
SERVER_OBJECT = server.o

# Define the executable
SERVER = server

SOCKET_SERVER = rpc_socket_server
RPC_SERVER = rpc_server

SOURCES.x = filemanager.x

TARGETS_SVC.c = filemanager_svc.c filemanager_xdr.c rpc_server.c
TARGETS_CLNT.c = filemanager_clnt.c filemanager_xdr.c rpc_socket_server.c
TARGETS = filemanager.h filemanager_xdr.c filemanager_clnt.c filemanager_svc.c filemanager_client.c filemanager_server.c

OBJECTS_CLNT = $(TARGETS_CLNT.c:%.c=%.o)
OBJECTS_SVC = $(TARGETS_SVC.c:%.c=%.o)

# Compiler flags 
CPPFLAGS += -D_REENTRANT
CFLAGS += -g 
LDLIBS += -lnsl -lpthread -I/usr/include/tirpc -ltirpc
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

.SECONDARY: $(OBJECTS_CLNT) $(OBJECTS_SVC)

$(OBJECTS_CLNT): %.o: %.c
	$(LINK.c) $(LDLIBS) -c $< -o $@

$(filter-out filemanager_xdr.o,$(OBJECTS_SVC)): %.o: %.c
	$(LINK.c) $(LDLIBS) -c $< -o $@

$(RPC): $(TARGETS)
$(TARGETS) : $(SOURCES.x) 
	rpcgen $(RPCGENFLAGS) $(SOURCES.x)

$(OBJECTS_CLNT) : $(TARGETS_CLNT.c) 

$(OBJECTS_SVC) : $(TARGETS_SVC.c) 

$(SOCKET_SERVER) : $(OBJECTS_CLNT) 
	$(LINK.c) -o $(SOCKET_SERVER) $(OBJECTS_CLNT) $(LDLIBS) 

$(RPC_SERVER) : $(OBJECTS_SVC) 
	$(LINK.c) -o $(RPC_SERVER) $(OBJECTS_SVC) $(LDLIBS)

$(SERVER): $(SERVER_OBJECT)
	$(LINK.c) $(CFLAGS) -o $@ $^

$(SERVER_OBJECT): server.c
	$(LINK.c) $(CFLAGS) -c $< -o $@

 clean:
	 @$(RM) core $(TARGETS) $(OBJECTS_CLNT) $(OBJECTS_SVC) $(SOCKET_SERVER) $(RPC_SERVER)
	 @$(RM) -f Makefile.*
