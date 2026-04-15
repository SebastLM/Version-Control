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



OpStatus recv_header(int sock, PacketHeader &header) {

  int bytes = recv_all(sock, &header, sizeof(PacketHeader));
  if (bytes <= 0) return OpStatus::TerminateOp;
  
  header.payload_size = ntohll(header.payload_size);
  header.desync = ntohl(header.desync);

  if (header.desync != DESYNC) {
    std::cerr << "Data is not aligned." << std::endl;
    OpStatus fail = OpStatus::FailedOp;
    send_all(sock, &fail, sizeof(fail));
    return OpStatus::FailedOp;
  }
  return OpStatus::SuccessOp;
}



/*
 TODO: as of now this function will need to be changed further.
      -> in case of an error in the FileContents op, we will just start the process over again
         We will need to seperate the chanegs in the future.
*/
int handle_op(int sock, bool is_dir) {
  
  uint8_t expected_op  = Op::FileName;
  std::string file_name;
  uint32_t name_len; 
  uint64_t file_size; // needed so we know when to stop our receving loop
  
  OpStatus status;
  bool success = false;

  while (1) {
    if (success) return 0;

    PacketHeader header;
    OpStatus state = recv_header(sock, header);
    if (state == OpStatus::TerminateOp) {
      return 0;
    } else if (state == OpStatus::FailedOp) continue;

    switch (header.operation) {

      case Op::FileName:
        if (expected_op == Op::FileName) {
          name_len = header.payload_size;
          // add the null character to the end of the string where we will store the file
          // this is the constructor of the std::string class
          file_name.assign(name_len, '\0');

          if (name_len == 0) return 1; // end of transfer
          std::cout << "lenght of file name " << name_len << std::endl;
          
          status = recv_file_name(sock, name_len, file_name, success, is_dir);
          if (status == OpStatus::SuccessOp) expected_op++;
          else return -1;
        }
        break;

      case Op::FileContents:
        if (expected_op == Op::FileContents) { 
          std::cout << "file name: |||" << file_name << "|||" << std::endl;

          file_size = header.payload_size;
          std::cout << "file size: " << file_size << std::endl;
          
          status = recv_file(sock, file_name, file_size);
          if (status == OpStatus::SuccessOp) success = true;
          else if (status == OpStatus::FailedOp) {
            OpStatus fail = OpStatus::FailedOp;
            send_all(sock, &fail, sizeof(fail));
          }
        }
        break;
      default:
        std::cerr << "Unknown operation:" << (int)header.operation 
          << "\nOperation unknown to current protocol.h"
          << std::endl;
    }
  }
  return 0;
}



OpStatus recv_file_name(int sock, uint32_t name_len, std::string &file_name, bool &success, bool is_dir) {
  //receive the actual file name
  recv_all(sock, file_name.data(), name_len);

  // making sure i am not overwriting an actual system file
  if (fs::exists(file_name)) {
    fs::perms perm = fs::status(file_name).permissions();
    if ((perm & fs::perms::owner_write) == fs::perms::none) {
      std::cout << "File is a read only system file. Aborting..." << std::endl;
      OpStatus fail = OpStatus::TerminateOp;
      send_all(sock, &fail, sizeof(fail));
      return OpStatus::TerminateOp;
    }
  }

  if (is_dir) {
    fs::create_directories(file_name);
    // fs::permissions(name, fs::perms::owner_write, fs::perm_options::remove);

    OpStatus ok = OpStatus::SuccessOp;
    send_all(sock, &ok, sizeof(ok));
    success = true;
    return OpStatus::SuccessOp;
  }

  OpStatus ok = OpStatus::SuccessOp;
  send_all(sock, &ok, sizeof(ok));
  return OpStatus::SuccessOp;
}




// the purpose of this function is to keep on receving the commited files from a user
OpStatus recv_file(int sock, std::string file_name, uint64_t file_size) {

  // making sure all direcories exist to create a file
  std::error_code er;
  fs::create_directories(fs::path(file_name).parent_path(), er);
  if (er) {
      std::cerr << "failed to create directories for file: " << file_name << "\n" 
                << er.message() << std::endl;
      return OpStatus::FailedOp;
  }
  // fs::permissions(file_ame, fs::perms::none);

  // initialize a output file stream to create or overwrite a file. writing data in binary mode
  std::ofstream file_out(file_name, std::ios::binary);
  if (!file_out) {
    std::cout << "failed to open output file stream" << std::endl;
    return OpStatus::FailedOp;
  }

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
  std::ifstream file_in(file_name, std::ios::binary);

  if (!file_in) {
    std::cout << "failed to reopen file for hashing" << std::endl;
    return OpStatus::FailedOp;
  } 

  unsigned int len_hash;
  unsigned char hash_buffer[32];

  if (!hashing(file_in, hash_buffer, len_hash)) {
    std::cout << "error calculating hash value" << std::endl;
    return OpStatus::FailedOp;
  }
  if (len_hash != 32) {
    std::cout << "unexpected hash length" << std::endl;
    return OpStatus::FailedOp;
  }
  
  unsigned char received_hash[32];
  recv_all(sock, received_hash, 32);

  OpStatus op_value = OpStatus::SuccessOp;
  if (std::memcmp(received_hash, hash_buffer, len_hash) != 0) {
    op_value = OpStatus::FailedOp;
    std::cout << "hashes for file " << file_name << " are different" << std::endl;
    send_all(sock, &op_value, sizeof(op_value));
    return OpStatus::FailedOp;
  }

  send_all(sock, &op_value, sizeof(op_value));
  /*
    TODO: Send the hash as well in case of an error, so we the sender can try to figure out the cause of the error
          maybe something was left out, and find out from where....
  */
  std::cout << "\n\n";

  return OpStatus::SuccessOp;
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
