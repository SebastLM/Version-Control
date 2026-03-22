#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>

#include <vector>



#include "staging.h"
#include "file_sender.h"
#include "protocol.h"
#include "send_all_recv_all.h"



int main(int argc, char* argv[]) { 

  
  if (argc < 2){
    std::cout << "error, usage should be ./(...) add,  or  ./(...) commit." << std::endl;
    std::cout << "see documentation for more" << std::endl;
    return EXIT_FAILURE;
  }
  if (strcmp(argv[1], "create") == 0) { 
    std::string project_dir = argv[2];
    create_project(project_dir);
  }
    
  else if (strcmp(argv[1], "add") == 0) {

    // vector constructor. Receives first iterator memory address and the past-the-end address 
    std::vector<std::string> files(argv + 2, argv + argc);

    add(files);
    return EXIT_SUCCESS;

  } else 
  
  if (strcmp(argv[1], "commit") == 0) {

    ssize_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      perror("error setting up socket\n");
      return EXIT_FAILURE;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(47195);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("connection failed");
      return EXIT_FAILURE;
    }

    MsgType protocol = MsgType::COMMIT_FILES;
    uint8_t protocol_tmp = static_cast<uint8_t>(protocol);

    send_all(sock, &protocol_tmp, sizeof(protocol_tmp));
    commit(sock);
  }
}
