#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>
#include <string>

int send_files(std::string files_to_commit) { 

  std::ifstream file_info(files_to_commit, std::ios::binary);
  if (!file_info) {
    perror("failed to open file");
    exit(EXIT_FAILURE);
  }

  // const char *hello = "hello from client";

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
  
  std::string file_name_tmp;
  while (std::getline(file_info, file_name_tmp)) {
   /*
      TODO: implement the read line logic to convert it into a file for reading 
   */ 
    // const char* file_name = file_name_tmp.c_str();
    std::vector<char> file_name(file_name_tmp.begin(), file_name_tmp.end()); 
    file_name.push_back('\0');
    char * c = &file_name[0];

    std::ifstream file(files_name, std::ios::binary);
    if (!file) {
      perror("failed to open file");
      exit(EXIT_FAILURE);
    }

    size_t total = strlen(file_name) + 1; // +1 to guarantee that the file is null terminated \0
    size_t sent = 0;
    std::cout << "connected" << std::endl;
    while (sent < total) {
      ssize_t s = send(sock, file_name + sent, total - sent, 0);
      std::cout << "filename"  << std::endl;
      if (s <= 0) {
        perror("error sending file name");
        close(sock);
        exit(EXIT_FAILURE);
      }
      sent += s;
    }

    char buffer[1024*64];
  
    while (file) {
      file.read(buffer, sizeof(buffer));
      std::streamsize n = file.gcount();
      if (n == 0) break;
    
      std::streamsize sent = 0;

      while (sent < n) {

        ssize_t s = send(sock, buffer + sent, n - sent, 0);
        if (s <= 0) {
          perror("error sending file chunk");
          close(sock);
          file.close();
          exit(EXIT_FAILURE);
        }
        sent += s;
     }
    }

    file.close();
  }
  close(sock);
  exit(EXIT_SUCCESS);
}
