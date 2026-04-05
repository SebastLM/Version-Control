#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <filesystem>

#include "hash.h"
#include "send_all_recv_all.h"
#include "file_receiver.h"

#define MAX_CHUNK_SIZE 64 * 1024


/*
  this function is needed because uint64_t needs a 64-bit conversion for network to host translation
*/
uint64_t ntohll(uint64_t v) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((uint64_t)ntohl(v & 0xFFFFFFFF) << 32) | ntohl(v >> 32);
#else
    return v;
#endif
}


namespace fs = std::filesystem;



  
// the purpose of this function is to keep on receving the commited files from a user
int recv_file(int sock, bool is_dir) {

   
  // file name length
  uint32_t name_len;
  recv_all(sock, &name_len, sizeof(name_len)); 
  name_len = ntohl(name_len); 
  
  if (name_len == 0) return 1; // end of transfer
  
  std::cout << "lenght of file name " << name_len << std::endl;

  // add the null character to the end of the string where we will store the file
  // this is the constructor of the std::string class
  std::string name(name_len, '\0');

  //receive the actual file name
  recv_all(sock, name.data(), name_len);

  // making sure i am not overwriting an actual system file
  fs::perms perm = fs::status(name).permissions();
  if ((perm & fs::perms::owner_write) == fs::perms::none) {
    std::cout << "File is a read only system file. Aborting..." << std::endl;
    return 0;
  }

  if (is_dir) {
    fs::create_directory(name);
    fs::permissions(name, fs::perms::owner_write, fs::perm_options::remove);
    return 1;
  }
  std::cout << "file name: |||" << name << "|||" << std::endl;

  // needed so we know when to stop our receving loop
  uint64_t file_size;
  recv_all(sock, &file_size, sizeof(file_size));
  file_size = ntohll(file_size);
  std::cout << "file size: " << file_size << std::endl;

  // making sure all direcories exist to create a file
  // case: creating file /src/include/special/lala.h
  // if special didnt exist, when creating the file, an error would be generated
  // this way we safe guard the file creation
  std::error_code er;
  fs::create_directories(name, er);
  if (er) {
      std::cerr << "failed to create directories for file: " << name << "\n" 
                << er.message() << std::endl;
  }
  fs::permissions(name, fs::perms::none);

  // initialize a output file stream to create or overwrite a file. writing data in binary mode
  std::ofstream file_out(name, std::ios::binary);
  if (!file_out) throw std::runtime_error("failed to open output file stream");

  char file_buf[MAX_CHUNK_SIZE];
  int i = 1;
  while (file_size > 0) {
    // we do this to deal with the buffer not filling, and so we know the actual recived len
    size_t file_chunk = std::min<size_t>(sizeof(file_buf), file_size);
    recv_all(sock, file_buf, file_chunk);
    std::cout << "\treceived chunk: " << i << std::endl;

    file_out.write(file_buf, file_chunk);
    file_size -= file_chunk;
    i++;
  }
  file_out.close();
  std::ifstream file_in(name, std::ios::binary);

  if (!file_in)
    throw std::runtime_error("failed to reopen file for hashing");

  unsigned int len_hash;
  unsigned char hash_buffer[32];

  if (!hashing(file_in, hash_buffer, len_hash)) throw std::runtime_error("error calculating hash value");
  if (len_hash != 32) throw std::runtime_error("unexpected hash length");
  
  unsigned char received_hash[32];
  recv_all(sock, received_hash, 32);

  int op_value = 1;
  if (std::memcmp(received_hash, hash_buffer, len_hash) != 0) {
    op_value = 0;
    std::cout << "hashes for file " << name << " are different" << std::endl;
  }

  send_all(sock, &op_value, sizeof(op_value));
  /*
    TODO: add recv loop for waiting for file.
    // for this i will not close the out stream earlie, Will instead flush(file.flush()) ->
    // then in case of hash being different will need to move the pointer to the beggining for writing(so im not writing at the end but instead replacing the corrupted file)
  */
  std::cout << "\n\n";

  return 1;
}






int recv_removed_entry(int sock) {

  uint32_t name_len;
  recv_all(sock, &name_len, sizeof(name_len));

  name_len = ntohl(name_len);
 
  if (name_len == 0) return 1; // end of transfer
  
  std::string name(name_len, '\0');

  recv_all(sock, name.data(), name_len);
  std::cout << "removing from project: " << name << std::endl;
  std::cout << "\n\n";

  fs::path path_rm = name;
  std::error_code er;

  fs::remove_all(path_rm, er);
  if (er) 
    std::cerr << "failed to remove " << path_rm << ": " 
              << er.message() << std::endl;

  return 0;
}




/*
int file_receiver(int new_socket) {

   
  while(1) {
    int trasnfer_value = 0;
    trasnfer_value = send_all_recv_all::recv_files(new_socket);
    
    if (trasnfer_value) break;
    std::cout << "Error receiving files" << std::endl;
  }

  close(new_socket);

  std::cout << "\n\nsucess in transfering the files" << std::endl;
  return EXIT_SUCCESS;
  
}
*/
