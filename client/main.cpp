#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>

int main(int argc, char *argv[]) { 

  if (argc < 2) {
    std::cerr << "usage: ./exec file" << std::endl;
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
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
  
  const char* file_name = argv[1];
  send(sock, file_name, strlen(file_name) + 1, 0); // +1 to guarantee that the file is null terminated \0
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
  close(sock);
  exit(EXIT_SUCCESS);
}
