CC = gcc
CFLAGS = -Wall -Wextra -g 
LDFLAGS =-lrt -pthread -lnsl -lpthread -ltirpc -I/usr/include/tirpc

# Define object files
SERVER_OBJECT = server.o

# Define the executable
SERVER = server

# Define all rules
all:
	@clear
	@make -s clean
	@make -s $(SERVER)
	@echo "Compiled server"

$(SERVER): $(SERVER_OBJECT)
	$(LINK.c) $(CFLAGS) -o $@ $^

$(SERVER_OBJECT): server.c
	$(LINK.c) $(CFLAGS) -c $< -o $@

clean:
	@rm -f files/*
	@rm -f *.csv
	@rm -f *.json
	@rm -f $(SERVER_OBJECT) $(SERVER)
	@echo "Cleaned up"
