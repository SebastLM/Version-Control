#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <iostream>

#include "file_sender.h"
#include "file_receiver.h"
#include "protocol.h"
#include "send_all_recv_all.h"

int main  () {
   int port = 47195 , new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  int val = 1;

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

  while (1) {
    uint8_t protocol_tmp;
    recv_all(new_socket, &protocol_tmp, sizeof(protocol_tmp));

    MsgType protocol = static_cast<MsgType>(protocol_tmp);
    switch (protocol) {

      case MsgType::CREATE_PROJECT:
        printf("still on work\n");
        break;



      // in case the remote peer wants to commit 
      case MsgType::COMMIT_FILES:

        int line = 0;
        bool end = false;
        while(1) {

          if (end) break;

          Action action;
          recv_all(new_socket, &action, sizeof(action));
          line ++;

          switch (action) {

            case Action::Remove:
              if (recv_removed_entry(new_socket))
                end = true;
              break;
            
            case Action::AddFile:
              if (recv_file(new_socket, 0))
                end = true;
              break;
            
            case Action::AddDir:
              if (recv_file(new_socket, 1))
                end = true;
              break;

            default:
              std::cout << "Undefined behaviour, error in line " << line << "of stage file " << std::endl 
                        << "consider manualy fixing the line" << std::endl;
              break;
          }
        }

        close(new_socket); 
        break;

      case MsgType::PULL_FILES:
        printf("still on work\n");
        break;

      default:
        throw std::runtime_error("message type provided not know");
    }
  }
}
