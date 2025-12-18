#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() { 
  
  struct sockaddr_in serv_addr;
  const char *hello = "hello from client";
  char buffer[1024] = {0};

  ssize_t sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("error setting up socket\n");
    exit(EXIT_FAILURE);
  }

  serv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, "192.168.1.95", &serv_addr.sin_addr);
  serv_addr.sin_port = htons(47195);
  
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("connection failed");
    exit(EXIT_FAILURE);
  }
  
  send(sock, hello, strlen(hello), 0);
  printf("hello message sent\n");

  do {
    recv(sock, buffer, 1024, 0);
    printf("%s\n", buffer);
  } while(1);
}
