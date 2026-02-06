CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS  := -lcrypto

CLIENT_DIR := client
SERVER_DIR := server
SRC_DIR    := src
OBJ_DIR    := obj


COMMON_SRC := $(SRC_DIR)/send_all_recv_all.cpp \
$(SRC_DIR)/hash.cpp \
$(SRC_DIR)/file_receiver.cpp \
$(SRC_DIR)/file_sender.cpp \
$(SRC_DIR)/path_trim.cpp

CLIENT_FILES := $(CLIENT_DIR)/client_main.cpp \
$(CLIENT_DIR)/add.cpp \
$(CLIENT_DIR)/commit.cpp \
$(CLIENT_DIR)/index.cpp

SERVER_FILES := $(SERVER_DIR)/server_main.cpp


CLIENT_OBJS := $(COMMON_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/$(SRC_DIR)/%.o) \
               $(CLIENT_FILES:$(CLIENT_DIR)/%.cpp=$(OBJ_DIR)/$(CLIENT_DIR)/%.o)

CLIENT_OBJS := $(CLIENT_OBJS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/$(SRC_DIR)/%.o)

SERVER_OBJS := $(COMMON_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/$(SRC_DIR)/%.o) \
               $(SERVER_FILES:$(SERVER_DIR)/%.cpp=$(OBJ_DIR)/$(SERVER_DIR)/%.o)


CLIENT_EXE := $(CLIENT_DIR)/client
SERVER_EXE := $(SERVER_DIR)/server

all: $(CLIENT_EXE) $(SERVER_EXE)

$(CLIENT_EXE): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_EXE): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(CLIENT_EXE) $(SERVER_EXE)

.PHONY: all clean
