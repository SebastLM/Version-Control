#include <iostream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

int main() {
  int port = 47195 , new_socket, valread;
  struct sockaddr_in address;
  char buffer[1024] = {0};
  const char* hello = "recived message from server";

  ssize_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1 ) {
    perror("socket set up failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int)) <  0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET; // address format set to host and port name
  address.sin_addr.s_addr = INADDR_ANY; // address to listen on all network interfaces 
  address.sin_port = htons(port);

  if (bind(server_socket, (struct sockaddr*) address, sizeof(address)) < 0) {
    perror("error binding socket to port: %d", port);
    exit(EXIT_FAILURE);
  }
  
  if (listen(server_socket, 2) < 0) {
    perror("error on listening for incoming connections");
    exit(EXIT_FAILURE);
  }
  
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
    perror("error accepting connections");
    exit(EXIT_FAILURE);
  }
  
  valread = read(new_socket, buffer, 1024);
  printf("%s", buffer);

  do {
    send(new_socket, hello, strlen(hello), 0);
    /* 
      in send we dont need to specifie an ip,
      since this is based on establishing connections is just what we need
    */
  } while (1);
  
}
