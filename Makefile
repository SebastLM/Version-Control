# Compiler
CXX := g++

# Compiler flags
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2 -Iinclude

# Linker flags (OpenSSL)
LDFLAGS := -lcrypto

# Directories
CLIENT_DIR := client
SERVER_DIR := server
SRC_DIR    := src

# Targets
CLIENT := $(CLIENT_DIR)/client
SERVER := $(SERVER_DIR)/server

# Shared source files
COMMON_SRC := \
	$(SRC_DIR)/send_all_recv_all.cpp \
	$(SRC_DIR)/hash.cpp \
	$(SRC_DIR)/file_receiver.cpp \
	$(SRC_DIR)/file_sender.cpp

# Client / server sources
CLIENT_SRC := $(CLIENT_DIR)/client_main.cpp $(COMMON_SRC)
SERVER_SRC := $(SERVER_DIR)/server_main.cpp $(COMMON_SRC)


# Default target
all: $(CLIENT) $(SERVER)

# Build client
$(CLIENT): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build server
$(SERVER): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Clean
clean:
	rm -f $(CLIENT) $(SERVER)

.PHONY: all clean
