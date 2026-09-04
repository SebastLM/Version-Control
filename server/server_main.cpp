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


/*
  TODO: implement threading 
*/



int commit_action(int new_socket) {

  int line = 0;
  while(1) {

    Action action;
    if (recv_all(new_socket, &action, sizeof(action)) <= 0) {
      std::cerr << "Fatal: Connection lost." << std::endl;
      return -1;
    }

    line ++;
    
    if (action == Action::EndCommit) {
      std::cout << "End of Commit." << std::endl;
      return 0;
    }
    
    int result = 0;
    switch (action) {

      case Action::Remove:
        result = recv_removed_entry(new_socket);
        break;
      
      case Action::AddFile:
        result = recv_file(new_socket, 0);
        break;
      
      case Action::AddDir:
        result = recv_file(new_socket, 1);
        break;
      
      case Action::EndCommit:
        std::cout << "Commit was done successfully." << std::endl;
        return 0;

      default:
        std::cerr << "FATAL: Protocol desync at line " << line << " of user stage file" << std::endl;
        break;
    }
    
    if (result == 1) {
      std::cout << "WARNING: Undefined behaviour, error in line **" << line << "** of stage file " << std::endl 
                  << "consider manualy fixing the line" << std::endl;
    }

  }
  return 0;
}






int main  () {
  int port = 47195 , new_socket;

  ssize_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("socket set up failed");
    exit(EXIT_FAILURE);
  }
  
  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(opt), sizeof(opt)) <  0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address;
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
  
  int addrlen = sizeof(address);
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
      { 
        commit_action(new_socket);

        close(new_socket); 
        break;
      }

      case MsgType::PULL_FILES:
        printf("still on work\n");
        break;

      default:
        throw std::runtime_error("message type provided not know");
    }
  }
}















/*TODO: IMP: USE space_info from filesystem to find space in the filesystem to know where to store the files*/
