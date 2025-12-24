#include "send_all_recv_all.h"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <netinet/in.h>
#include <sys/socket.h>


void send_all(int sock, const void* data, size_t len) {
  const char* p = static_cast<const char*>(data);
  
  while (len > 0) {
    ssize_t send_size = send(sock, p, len, 0);
    if (send_size < 0) throw std::runtime_error("failed to send file");

    p += send_size;
    len -= send_size;
  }
}


void recv_all(int sock, void* data, size_t len) {
  char* p = static_cast<char*>(data);

  while (len > 0) {
    ssize_t recv_size = recv(sock, p, len, 0);
    if (recv_size <= 0) throw std::runtime_error("failed to recive file");

    p += recv_size;
    len -= recv_size;
  }
}
