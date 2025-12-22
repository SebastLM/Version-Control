#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>



int main() {

  int port = 47195 , new_socket;
  struct sockaddr_in address;
  unsigned long LEN = 1024 * 64;
  char buffer[LEN] = {0};
  int addrlen = sizeof(address);
  // const char* hello = "recived message from server";

  ssize_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1 ) {
    perror("socket set up failed");
    exit(EXIT_FAILURE);
  }
  
  struct timeval tv;
  tv.tv_sec = 5;      // timeout = 5 second
  tv.tv_usec = 0;
  
  if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <  0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET; // address format set to host and port name
  address.sin_addr.s_addr = INADDR_ANY; // address to listen on all network interfaces 
  address.sin_port = htons(port);

  if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("error binding socket to port");
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  

  /* 
  listen:
    mark the socket as a passive socket, with the second arg being the maximum length to which the the queue of connections may grow
  */
  if (listen(server_socket, 2) < 0) {
    perror("error on listening for incoming connections");
    exit(EXIT_FAILURE);
  }
  
  if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
    perror("error accepting connections");
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  
  std::string file_name;
  char ch;
  bool ended_transfer = 0;

  while (1) {

    int iter = 0;
    while (true) {
      ssize_t r = recv(new_socket, &ch, 1, 0);
      if (r < 0) {
         if ((errno == EAGAIN || errno == EWOULDBLOCK) && iter == 0) {
          std::cout << "endend the file transfer";
          ended_transfer = 1;
          break;
        }
        perror("failed to receive filename");
        exit(EXIT_FAILURE);
      }
      iter++;
      if (ch == '\0') break;
      file_name.push_back(ch);
    }

    if (ended_transfer) break;

    std::ofstream file(file_name, std::ios::binary);
  
    if (!file.is_open()) {
      perror("error opening the file for writing");
      close(server_socket);
      close(new_socket);
      exit(EXIT_FAILURE);
    }

    ssize_t recv_len;

    do {

      recv_len = recv(new_socket, buffer, sizeof(buffer), 0);
      if (recv_len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)  std::cout << "endend the file transfer missing files" << std::endl;
  
        file.close();
        perror("error reciving file chunk\n");
        close(server_socket);
        close(new_socket);
        exit(EXIT_FAILURE);
      }
    
      file.write(buffer, recv_len); // already advances the pointer in where we are writing in the file

      char ip_buffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, INET_ADDRSTRLEN);
      std::string ip_string = ip_buffer;
      std::cout << "received chunk from IP address: " << ip_string << "\n" << std::endl;
      /*
        in send we dont need to specifie an ip,
        since this is based on establishing connections is just what we need
      */
    } while (recv_len > 0);

    file.close();
  }
  close(server_socket);
  close(new_socket);

  std::cout << "sucess in transfering the files" << std::endl;
  return EXIT_SUCCESS;

}
