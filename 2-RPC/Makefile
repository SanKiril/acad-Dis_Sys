

SOCKET_SERVER = server
RPC_SERVER = rpc_server

SOURCES.x = filemanager.x

TARGETS_SVC.c = filemanager_svc.c filemanager_xdr.c rpc_server.c
TARGETS_CLNT.c = filemanager_clnt.c filemanager_xdr.c server.c
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
	@make -s clean
	@make -s $(SOCKET_SERVER)
	@echo "Compiled socket server"
	@make -s $(RPC_SERVER)
	@echo "Compiled rpc server"

$(TARGETS) : $(SOURCES.x) 
	rpcgen $(RPCGENFLAGS) $(SOURCES.x)

$(OBJECTS_CLNT) : $(TARGETS_CLNT.c) 

$(OBJECTS_SVC) : $(TARGETS_SVC.c) 

$(SOCKET_SERVER) : $(OBJECTS_CLNT) 
	$(LINK.c) -o $(SOCKET_SERVER) $(OBJECTS_CLNT) $(LDLIBS) 

$(RPC_SERVER) : $(OBJECTS_SVC) 
	$(LINK.c) -o $(RPC_SERVER) $(OBJECTS_SVC) $(LDLIBS)

 clean:
	 @$(RM) core $(TARGETS) $(OBJECTS_CLNT) $(OBJECTS_SVC) $(SOCKET_SERVER) $(RPC_SERVER)

