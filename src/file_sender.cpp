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
#include "file_sender.h"
#include "path_trim.h"

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

  
int send_files(int sock, std::string& file_to_send) {
  
  std::ifstream file(file_to_send, std::ios::binary);
  if (!file && !file_to_send.empty()) {
    printf("opening the file for commit failed\n");
    return 0;
  }

  // file_name(file_to_send);
  std::cout << file_to_send << std::endl;

  uint32_t name_len = file_to_send.size();
  uint32_t net_name_len = htonl(name_len);

  if (!file_to_send.empty()) 
    std::cout << "\tsending " << file_to_send << " name length: " << file_to_send.size() << std::endl;

  send_all(sock, &net_name_len, sizeof(net_name_len));
  
  if (file_to_send.empty()) {
    file.close();
    return 1;
  }
  std::cout << "\tsending file name " <<  file_to_send << std::endl;
  send_all(sock, file_to_send.data(), name_len);
  
  // allow for seaking a position in a file
  // its included in the fstream header
  file.seekg(0, std::ios::end); // set the position to the read in the stream 0 from the end(std::ios::end), so basically the file pointer is now at the end of the file
  uint64_t size_file = file.tellg(); // used to find current read position, which can tell us the total file size;
  file.seekg(0); // back to the beggining for reading the file and transfering
  
  std::cout << "\tsending file size: " << size_file << std::endl;
  size_file = htonll(size_file);
  send_all(sock, &size_file, sizeof(size_file));// sending the size of the file so it knows how much it will take

  char file_buf[MAX_CHUNK_SIZE];
  int i = 1; 
  while (file) {

    file.read(file_buf, sizeof(file_buf));
    std::cout << "\t\tsending chuck: " << i << " (" << file_to_send << ")" << std::endl;
    send_all(sock, file_buf, file.gcount());
    i++;
  }

  unsigned int len_hash;
  unsigned char hash_buffer[32];
  
  // moving the read pointer for the beggining for proper hashing
  file.clear();
  file.seekg(0, std::ios::beg);

  if (!hashing(file, hash_buffer,len_hash)) throw std::runtime_error("failed to calculate hash");
  if (len_hash != 32) throw std::runtime_error("unexpected hash length");

  size_t hash_len = len_hash;
  int value;
  // if the hash is not the same we will have to repeat the process of sending the file
  // this is probably not going to happen since we are using TCP, but we want to be 100% sure
  // the process of sending the file again from the beggining might be slow but since its probably never gonna happen and if it does its rare
  // i would prefer to only check at the end rather then be checkin in each exchange, making me check 4+ times
  send_all(sock, hash_buffer, hash_len);
  recv_all(sock, &value, sizeof(int));
  if (!value) {
    file.close();
    return 0;
  }

  std::cout << "\n\n";
  file.close();
  return 1;
}

}



int file_sender(int sock, std::string files_to_commit) {

  std::ifstream file_info(files_to_commit, std::ios::binary);
  if (!file_info) {
    perror("failed to open file");
    exit(EXIT_FAILURE);
  }

  if (!file_info.is_open()) {
    perror("failed to open Files to commit\n");
    return EXIT_FAILURE;
  }
  
  std::string file_name;
  while (std::getline(file_info, file_name)) {
    while (1) {
      if (!file_name.empty()) // making sure we dont print when we in the last line. last line has nothing but its our way of stoping the receiver from waiting on more files
        std::cout << "preparing to send file: ||| " << file_name << " |||" << std::endl;
   /*
      read line logic to convert it into a file for reading 
   */ 
    
    int transfer_value = 0; 
    transfer_value = send_all_recv_all::send_files(sock, file_name);

    if (transfer_value) break;
    std::cerr << "error sending the file" << file_name << ",trying again" << std::endl;
    }
  }
  std::cout << "\n\nending the file transfer successfully" << std::endl;
  file_info.close();
  close(sock);
  exit(EXIT_SUCCESS);
}
