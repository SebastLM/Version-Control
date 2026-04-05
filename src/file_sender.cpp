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
#include "file_locker.h"

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



// objects needed for locking
SystemWideLock sys_lock;
ThreadLock thread_Lock;


/*
  Why do i not use the host to network translation every where i send? because the Endianness conversion is only for fixed width integers that represent numbers
  like uint32_t and uint64_t. File names and its contents or buffers cant be convert to so
*/
int send_file(int sock, const std::string& file_to_send, bool is_dir) {
 
  
  // locks the file for reading (false == reading)
  GuardLock guardLock(sys_lock, thread_Lock, file_to_send, false);

  std::ifstream file;

  if (!is_dir) {
    file.open(file_to_send, std::ios::binary);

    if (!file && !file_to_send.empty()) {
      printf("opening the file for commit failed\n");
      return 0;
    }
  }

  // file_name(file_to_send);
  std::cout << file_to_send << std::endl;

  uint32_t name_len = file_to_send.size();
  uint32_t net_name_len = htonl(name_len);

  if (!is_dir) {
    if (!file_to_send.empty()) 
      std::cout << "\tsending " << file_to_send << " name length: " << file_to_send.size() << std::endl;
  }
  
  if (send_all(sock, &net_name_len, sizeof(net_name_len)) < 0) {
    std::cerr << "Failed to send Name_len of " << file_to_send << std::endl;
    return 0; 
  }
  
  if (file_to_send.empty()) {
    file.close();
    return 1;
  }
 
  std::cout << "\tsending file name " <<  file_to_send << std::endl;
  if (send_all(sock, file_to_send.data(), name_len) < 0) {
    std::cerr << "Failed to send file_name " << file_to_send << std::endl;
    return 0;
  }

  // in case we are sending a dir, nothing more is needed
  if (is_dir)
    return 1;


  // allow for seaking a position in a file
  // its included in the fstream header
  file.seekg(0, std::ios::end); // set the position to the read in the stream 0 from the end(std::ios::end), so basically the file pointer is now at the end of the file
  uint64_t size_file = file.tellg(); // used to find current read position, which can tell us the total file size;
  file.seekg(0); // back to the beggining for reading the file and transfering
  
  std::cout << "\tsending file size: " << size_file << std::endl;
  size_file = htonll(size_file);
  if (send_all(sock, &size_file, sizeof(size_file)) < 0) {// sending the size of the file so it knows how much it will take
    std::cerr << "Failed to send size of file: " << file_to_send << std::endl;
    return 0; 
  }

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
    return 1;
  }

  std::cout << "\n\n";
  file.close();
  return 1;
}






int send_remove_entry(int sock, const std::string& entry_to_remove) {

  uint32_t name_len = entry_to_remove.size();
  uint32_t net_name_len = htonl(name_len);

  if (send_all(sock, &net_name_len, sizeof(net_name_len)) < 0) {
    std::cerr << "Failed to send Name_len of " << entry_to_remove << std::endl;
    return 0;
  }
  
  std::cout << "\tsending entry for removal " <<  entry_to_remove << std::endl;
  
  if (send_all(sock, entry_to_remove.data(), name_len) < 0) {
    std::cerr << "Failed to send Name of " << entry_to_remove << " to be removed" << std::endl;
    return 0; 
  }
  
  return 1;
}



