#include <arpa/inet.h>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include "file_receiver.h"
#include "file_sender.h"
#include "protocol.h"
#include "send_all_recv_all.h"

namespace fs = std::filesystem;

/*
  TODO: implement threading
*/

int commit_action(int new_socket) {

  int line = 0;
  while (1) {
    std::cerr << "DEBUG: waiting for action, line " << line << std::endl;
    Action action;
    if (recv_all(new_socket, &action, sizeof(action)) < 0) {
      std::cerr << "Fatal: Connection lost." << std::endl;
      return -1;
    }

    line++;

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
      std::cerr << "FATAL: Protocol desync at line " << line
                << " of user stage file" << std::endl;
      break;
    }

    if (!result) {
      std::cout << "WARNING: Undefined behaviour, error in line **" << line
                << "** of stage file " << std::endl
                << "consider manualy fixing the line" << std::endl;
    }
  }
  return 0;
}

// receives the project name
// runs the commit
void handle_commit(int new_socket) {

  uint32_t name_len_net;
  if (recv_all(new_socket, &name_len_net, sizeof(name_len_net)) < 0) {
    std::cerr << "Failed to receive project name length." << std::endl;
    return;
  }

  uint32_t proj_name_len = ntohl(name_len_net);
  std::string project_name(proj_name_len, '\0');

  if (proj_name_len > 0 &&
      recv_all(new_socket, project_name.data(), proj_name_len) < 0) {
    std::cerr << "Failed to receive project name." << std::endl;
    return;
  }

  std::string original_cwd = fs::current_path().string();

  if (!project_name.empty()) {
    std::error_code ec;

    fs::create_directories(project_name, ec);
    if (ec) {
      std::cerr << "Failed to create project directory '" << project_name
                << "': " << ec.message() << std::endl;
      return;
    }

    fs::current_path(project_name, ec);
    if (ec) {
      std::cerr << "Failed to enter project directory '" << project_name
                << "': " << ec.message() << std::endl;
      return;
    }
  }

  std::cerr << "DEBUG: committing into project: " << project_name << std::endl;

  try {
    commit_action(new_socket);
  } catch (const std::exception &e) {
    std::cerr << "Commit failed: " << e.what() << std::endl;
  }

  std::error_code restore_ec;
  fs::current_path(original_cwd, restore_ec);
  if (restore_ec) {
    std::cerr << "WARNING: failed to restore original working directory: "
              << restore_ec.message() << std::endl;
  }
}

int main() {
  int port = 47195, new_socket;

  ssize_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("socket set up failed");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(opt), sizeof(opt)) <
      0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET; // address format set to host and port name
  address.sin_addr.s_addr =
      INADDR_ANY; // address to listen on all network interfaces
  address.sin_port = htons(port);

  if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("error binding socket to port");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  /*
  listen:
    mark the socket as a passive socket, with the second arg being the maximum
  length to which the the queue of connections may grow
  */
  if (listen(server_socket, 2) < 0) {
    perror("error on listening for incoming connections");
    exit(EXIT_FAILURE);
  }

  while (1) {
    int addrlen = sizeof(address);
    if ((new_socket = accept(server_socket, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      perror("error accepting connections");
      continue;
    }

    uint8_t protocol_tmp;
    if (recv_all(new_socket, &protocol_tmp, sizeof(protocol_tmp)) < 0) {
      close(new_socket);
      continue;
    }

    MsgType protocol = static_cast<MsgType>(protocol_tmp);
    switch (protocol) {

    case MsgType::CREATE_PROJECT:
      printf("still on work\n");
      close(new_socket);
      break;

    case MsgType::COMMIT_FILES:
      handle_commit(new_socket);
      close(new_socket);
      break;

    case MsgType::PULL_FILES:
      printf("still on work\n");
      break;

    default:
      std::cerr << "Unknown Message type received" << std::endl;
      close(new_socket);
      break;
    }
  }
}

/*TODO: IMP: USE space_info from filesystem to find space in the filesystem to
 * know where to store the files*/
