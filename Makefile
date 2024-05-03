CC = gcc
CFLAGS = -Wall -Wextra -g  # Add additional flags as needed

# Define object files
SERVER_OBJECT = server.o devtools.o  # devtools.o is temporary

# Define the executable
SERVER = server

# Define all rules
all:
	@make -s clean
	@clear
	@make -s $(SERVER)
	@echo "Compiled server"

test:
	@make -s all
	@make -s clean

$(SERVER): $(SERVER_OBJECT)
	$(CC) $(CFLAGS) -o $@ $^

server.o: server.c
	$(CC) $(CFLAGS) -c $< -o $@

devtools.o: devtools.c  # temporary
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f *.csv
	@rm -f $(SERVER_OBJECT) $(SERVER)
