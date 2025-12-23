#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <stdexcept>
#include <print>

#include "send_all_recv_all.h"

#define MAX_CHUNK_SIZE 64 * 1024

/*
 
  TODO: the files will be moved to a dir when changed.
  when i want to transfer the files i will simply send the files on that directory, for this i will need to have the server and me know what project we are commiting to
 
 */

int send_files(int sock, const std::string& file_to_send) {
  
  std::ifstream file(file_to_send, std::ios::binary);
  if (!file) throw std::runtime_error("opening the file for commit failed");

  uint32_t name_len = path.size();
  send_all(sock, &name_len, sizeof(name_len));
  send_all(sock, path.data(), name_len);
  
  // allow for seaking a position in a file
  // its included in the fstream header
  file.seekg(0, std::ios::end); // set the position to the read in the stream 0 from the end(std::ios::end), so basically the file pointer is now at the end of the file
  size_t size_file = file.tellg(); // used to find current read position, which can tell us the total file size;
  file.seekg(0); // back to the beggining for reading the file and transfering
  
  send_all(sock, &size_file, sizeof(size_file));// sending the size of the file so it knows how much it will take
  /*
   TODO: will had the "sending hash" here latter to make sure that the file is not altered 
  */

  char file_buf[MAX_CHUNK_SIZE];
  while (file) {
    file.read(file_buf, sizeof(file_buf));
    send_all(sock, file_buf, file.gcount());
  }
  file.close();
  return 1;
}




int send_files(std::string files_to_commit) { 


  // file that stores the "commit files"
  std::ifstream file_info(files_to_commit, std::ios::binary);
  if (!file_info) {
    perror("failed to open file");
    exit(EXIT_FAILURE);
  }

  ssize_t sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("error setting up socket\n");
    exit(EXIT_FAILURE);
  }
  
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "192.168.1.95", &serv_addr.sin_addr);
  serv_addr.sin_port = htons(47195);
  
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }
  
  if (!file_info.is_open()) {
    perror("failed to open Files to commit\n");
    return EXIT_FAILURE;
  }
  
  std::string file_name;
  while (std::getline(file_info, file_name)) {
    while (1) {
   /*
      read line logic to convert it into a file for reading 
   */ 
    int trasnfer_value = 0; 
    transfer_value = send_files(sock, file_name);

    if (trasnfer_value) break;
    std::print("error sending the file {}, trying again", file_name);
    }
  }

  file_info.close();
  close(sock);
  exit(EXIT_SUCCESS);
}
