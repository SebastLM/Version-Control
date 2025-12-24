#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "hash.h"
#include "send_all_recv_all.h"

#define MAX_CHUNK_SIZE 64 * 1024


/*
  this function is needed because uint64_t needs a 64-bit conversion for network to host translation
*/
uint64_t ntohll(uint64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((uint64_t)ntohl(v & 0xFFFFFFFF) << 32) | ntohl(v >> 32);
#else
    return v;
#endif
}




// doing a auxiliar function so that im able to handle further down the line more than 1 user
// the purpose of this function is to keep on receving the commited files from a user
//
namespace send_all_recv_all {

int recv_files(int sock) {

  while (true) {
    
    // file name length
    uint32_t name_len;
    recv_all(sock, &name_len, sizeof(name_len));
    
    name_len = ntohl(name_len);
    if (name_len == 0) break; // end of transfer
  
    // add the null character to the end of the string where we will store the file
    // this is the constructor of the std::string class
    std::string name(name_len, '\0');
    //receive the actual file name and add it to the fi
    recv_all(sock, name.data(), name_len);

    // needed so we know when to stop our receving loop
    uint64_t file_size;
    recv_all(sock, &file_size, sizeof(file_size));
    file_size = ntohll(file_size);

    // initialize a output file stream to create or overwrite a file. writing data in binary mode
    std::ofstream file_out(name, std::ios::binary);
    if (!file_out) throw std::runtime_error("failed to open output file stream");

    char file_buf[MAX_CHUNK_SIZE];
    while (file_size > 0) {
      // we do this to deal with the buffer not filling, and so we know the actual recived len
      size_t file_chunk = std::min<size_t>(sizeof(file_buf), file_size);
      recv_all(sock, file_buf, file_chunk);
      /*
        TODO: need to make sure i am not overwriting an actual system file;
      */
      file_out.write(file_buf, file_chunk);
      file_size -= file_chunk;
    }
  }
  return 1;
} 
} // namespace send_all_recv_all


int main() {

  int port = 47195 , new_socket;
  struct sockaddr_in address;
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
    trasnfer_value = send_all_recv_all::recv_files(new_socket);
    
    if (trasnfer_value) break;
    /*
      TODO: send this information back to the host so he know he has to send files again
      in the future make sure we do this inside the recv_files loop where when we dont receive a file according to the hash we send back that we didnt receive the whole file
      this way we can guarantee that the transfer will be complete
    */
    std::cout << "Error receiving files" << std::endl;
  }

  close(server_socket);
  close(new_socket);

  std::cout << "sucess in transfering the files" << std::endl;
  return EXIT_SUCCESS;
  
}
