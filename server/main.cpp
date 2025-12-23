#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <string>
#include <stdexcept>

#include "hash.h"
#include "send_all_recv_all.h"

#define MAX_CHUNK_SIZE 64 * 1024

// doing a auxiliar function so that im able to handle further down the line more than 1 user
// the purpose of this function is to keep on receving the commited files from a user
//
int recv_files(int sock) {

  while (true) {
    
    // file name length
    uint32_t name_len;
    recv_all(sock, &name_len, sizeof(name_len))

    if (name_len == 0) break; // end of transfer
  
    // add the null character to the end of the string where we will store the file
    // this is the constructor of the std::string class
    std::string name(name_len, '\0');
    //receive the actual file name and add it to the fi
    recv_all(sock, name.data(), name_len);
    
    // needed so we know when to stop our receving loop
    uint64_t file_size;
    recv_all(sock, &file_size, sizeof(file_size));
    
    // initialize a output file stream to create or overwrite a file. writing data in binary mode
    std::ofstream file_out(name, std::ios::binary);
    if (!file_out) throw std::runtime_error("failed to open output file stream");

    char file_buf[MAX_CHUNK_SIZE];
    while (size > 0) {
      // we do this to deal with the buffer not filling, and so we know the actual recived len
      size_t file_chunk = std::min<size_t>(sizeof(file_buf), file_size);
      recv_all(sock, file_buf, file_chunk);
      out.write(file_out);
      size -= file_chunk;
    }
  }
}



int main() {

  int port = 47195 , new_socket;
  struct sockaddr_in address;
  unsigned long LEN = 1024 * 64;
  char buffer[LEN] = {0};
  int addrlen = sizeof(address);
  int val = 1;
  // const char* hello = "recived message from server";

  ssize_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1 ) {
    perror("socket set up failed");
    exit(EXIT_FAILURE);
  }
  
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(val), sizeof(int)) <  0) {
    /*
      SOL_REUSEPORT allows for multiple listeing sockets, the kernel then balances the incoming connections
     */
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
  

  // accepted sockets can be many while listening tend to be only one
  if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
    perror("error accepting connections");
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  
  // loop for receiving files
  while(1) {
    int trasnfer_value = 0;
    trasnfer_value = recv_files(sock);
    
    if (trasnfer_value) break;
    /*
      TODO: send this information back to the host so he know he has to send files again
      in the future make sure we do this inside the recv_files loop where when we dont receive a file according to the hash we send back that we didnt receive the whole file
      this way we can guarantee that the transfer will be complete
    */
    std::cout << "Error receiving files" << std::endl;
  }
  /*
  while (1) {
   
    std::string file_name;
    char ch;
    
    while (true) {
      ssize_t r = recv(new_socket, &ch, 1, 0);
      if (r < 0) {
        perror("failed to receive filename");
        exit(EXIT_FAILURE);
      }
      if (ch == '\0') break;
      file_name.push_back(ch);
    }
    
      TODO: implement the logic of sending last "end" for the program to end. Will have to deal with the case of not receiving the end and being stuck on a infinite loop
    
    if (file_name == "end") break;

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
      
      // in send we dont need to specifie an ip,
      // since this is based on establishing connections is just what we need
      
    } while (recv_len > 0 && recv_len < 1024*64);

    file.close();
  }
  */
  close(server_socket);
  close(new_socket);

  std::cout << "sucess in transfering the files" << std::endl;
  return EXIT_SUCCESS;
  
}
