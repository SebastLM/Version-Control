#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "send_all_recv_all.h"
#include "hash.h"

#define MAX_CHUNK_SIZE 64 * 1024

/*
 
  TODO: the files will be moved to a dir when changed.
  when i want to transfer the files i will simply send the files on that directory, for this i will need to have the server and me know what project we are commiting to
 
 */

/* 
  function for host to  network translation for uint64_t
  extra:  no need for handling 128 bit file sizes, its unrealistic, 2^128 file_size?? absurd
  this conversion is needed because diferent systems store multi byte integers in different byte orders
*/
uint64_t htonll(uint64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((uint64_t)htonl(v & 0xFFFFFFFF) << 32) | htonl(v >> 32);
#else
    return v;
#endif
}



/*
  Why do i not use the host to network translation every where i send? because the Endianness conversion is only for fixed width integers that represent numbers
  like uint32_t and uint64_t. File names and its contents or buffers cant be convert to so
*/
namespace send_all_recv_all {
  
int send_files(int sock, const std::string& file_to_send) {
  
  std::ifstream file(file_to_send, std::ios::binary);
  if (!file) throw std::runtime_error("opening the file for commit failed");

  uint32_t name_len = htonl(file_to_send.size());

  std::cout << "sending " << file_to_send << " name length: " << file_to_send.size() << std::endl;
  send_all(sock, &name_len, file_to_send.size());
  
  std::cout << "sending file name " <<  file_to_send << std::endl;
  send_all(sock, file_to_send.data(), file_to_send.size());
  
  // allow for seaking a position in a file
  // its included in the fstream header
  file.seekg(0, std::ios::end); // set the position to the read in the stream 0 from the end(std::ios::end), so basically the file pointer is now at the end of the file
  uint64_t size_file = file.tellg(); // used to find current read position, which can tell us the total file size;
  file.seekg(0); // back to the beggining for reading the file and transfering
  
  std::cout << "sending file size: " << size_file << std::endl;
  size_file = htonll(size_file);
  send_all(sock, &size_file, sizeof(size_file));// sending the size of the file so it knows how much it will take
  /*
   TODO: will had the "sending hash" here latter to make sure that the file is not altered 
  */

  char file_buf[MAX_CHUNK_SIZE];
  int i = 1; 
  while (file) {

    file.read(file_buf, sizeof(file_buf));
    std::cout << "sending chuck: " << i << " (" << file_to_send << ")" << std::endl;
    send_all(sock, file_buf, file.gcount());
    i++;
  }
  file.close();
  return 1;
}

}


int main(int argc, char* argv[]) { 
  
  if (argc != 2){
    std::cout << "error, usage should be ./(...) file_with_commits" << std::endl;
    return EXIT_FAILURE;
  }

  std::string files_to_commit = argv[1];

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
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
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
      std::cout << "preparing to send file: " << file_name << std::endl;
   /*
      read line logic to convert it into a file for reading 
   */ 
    int transfer_value = 0; 
    transfer_value = send_all_recv_all::send_files(sock, file_name);

    if (transfer_value) break;
    std::cerr << "error sending the file" << file_name << ",trying again" << std::endl;
    }
  }

  file_info.close();
  close(sock);
  exit(EXIT_SUCCESS);
}
