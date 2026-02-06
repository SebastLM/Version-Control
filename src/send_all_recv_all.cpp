#include "send_all_recv_all.h"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <netinet/in.h>
#include <sys/socket.h>


int send_all(int sock, const void* data, size_t len) {
  const char* p = static_cast<const char*>(data);
  
  while (len > 0) {
    ssize_t send_size = send(sock, p, len, 0);

    if (send_size <= 0) {
      /*
        TODO: handle errors properly 
      */
      std::cout << "failed to send" << std::endl;
      return -1;
    }

    p += send_size;
    len -= send_size;
  }
  return 0;
}


int recv_all(int sock, void* data, size_t len) {
  char* p = static_cast<char*>(data);

  while (len > 0) {
    ssize_t recv_size = recv(sock, p, len, 0);
    if (recv_size <= 0) return -1;

    p += recv_size;
    len -= recv_size;
  }
  return 0;
}
