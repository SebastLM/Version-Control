#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

#include <vector>

#include "file_sender.h"
#include "protocol.h"
#include "send_all_recv_all.h"
#include "staging.h"
#include <unistd.h>

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

std::string find_pi_ip() {
  for (int i = 1; i < 254; ++i) {
    std::string candidate_ip = "10.55.0." + std::to_string(i);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
      continue;

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(47195);
    inet_pton(AF_INET, candidate_ip.c_str(), &serv_addr.sin_addr);

    const char *eth_interface = "eth0";
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, eth_interface,
               strlen(eth_interface));

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1) {
      int sockerr;
      socklen_t len = sizeof(sockerr);
      getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockerr, &len);
      if (sockerr == 0) {
        close(sock);
        return candidate_ip;
      }
    }
    close(sock);
  }
  return "";
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cout << "error, usage should be ./(...) add,  or  ./(...) commit."
              << std::endl;
    std::cout << "see documentation for more" << std::endl;
    return EXIT_FAILURE;
  }
  if (strcmp(argv[1], "create") == 0) {
    std::string project_dir = argv[2];
    create_project(project_dir);
  }

  else if (strcmp(argv[1], "add") == 0) {

    // vector constructor. Receives first iterator memory address and the
    // past-the-end address
    std::vector<std::string> files(argv + 2, argv + argc);

    add(files);
    return EXIT_SUCCESS;

  } else if (strcmp(argv[1], "commit") == 0) {

    std::cout << "finding Raspberry Pi on Ethernet..." << std::endl;
    std::string pi_ip = find_pi_ip();

    if (pi_ip.empty()) {
      std::cerr << "Error: Could not find Raspberry Pi on the Ethernet cable."
                << std::endl;
      return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      perror("error setting up socket\n");
      return EXIT_FAILURE;
    }

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(47195);
    inet_pton(AF_INET, pi_ip.c_str(), &serv_addr.sin_addr);

    const char *eth_interface = "eth0";
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, eth_interface,
               strlen(eth_interface));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("connection failed");
      close(sock);
      return EXIT_FAILURE;
    }

    MsgType protocol = MsgType::COMMIT_FILES;
    uint8_t protocol_tmp = static_cast<uint8_t>(protocol);
    send_all(sock, &protocol_tmp, sizeof(protocol_tmp));

    commit(sock);

    close(sock);
  }
}
