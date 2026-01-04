#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <string>

#include "file_sender.h"
#include "protocol.h"
#include "send_all_recv_all.h"

int main(int argc, char* argv[]) { 
  
  if (argc != 2){
    std::cout << "error, usage should be ./(...) file_with_commits" << std::endl;
    return EXIT_FAILURE;
  }

  // file that stores the "commit files"
  std::string files_to_commit = argv[1];

  ssize_t sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("error setting up socket\n");
    exit(EXIT_FAILURE);
  }
  
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  serv_addr.sin_port = htons(47195);
  
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }


  MsgType protocol = MsgType::COMMIT_FILES;
  uint8_t protocol_tmp = static_cast<uint8_t>(protocol);

  send_all(sock, &protocol_tmp, sizeof(protocol_tmp));
  file_sender(sock, files_to_commit);
}