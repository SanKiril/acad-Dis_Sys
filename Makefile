# Compiler
CC := gcc

# Compiler flags
CFLAGS := -Wall -Wextra -pedantic -g

# Directories
DIR_SRC := src
DIR_HDR := hdr
DIR_BIN := bin

# Library name
LIB_NAME := claves

# Linker flags
LDFLAGS := -lrt -pthread -L./$(DIR_BIN) -l$(LIB_NAME)

# Source files
SRC_SERVER := $(DIR_SRC)/server.c $(DIR_SRC)/services.c

# Dependencies
DEP_LIB := $(DIR_HDR)/claves.h $(DIR_HDR)/constants.h
DEP_SERVER := $(DIR_HDR)/messages.h $(DIR_HDR)/services.h $(DIR_HDR)/constants.h
DEP_CLIENT = $(DIR_HDR)/claves.h $(DIR_HDR)/constants.h

# Executables
BIN_LIB := $(DIR_BIN)/libclaves.so
BIN_SERVER := $(DIR_BIN)/servidor
BIN_CLIENT := $(DIR_BIN)/cliente


# Comiplation rules & dependencies
all: $(BIN_LIB) $(BIN_SERVER) $(BIN_CLIENT)

$(BIN_LIB): $(SRC_LIB) $(DEP_LIB)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(SRC_LIB)

$(BIN_SERVER): $(SRC_SERVER) $(DEP_SERVER)
	$(CC) $(CFLAGS) -o $@ $(SRC_SERVER)

$(BIN_CLIENT): $(SRC_CLIENT) $(DEP_CLIENT)
	$(CC) $(CFLAGS) -o $@ $(SRC_CLIENT) $(LDFLAGS)

# Compilation clean up
clean:
	rm -f $(DIR_BIN)/*
