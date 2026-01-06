#include <fstream>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>


#include "hash.h"

// hashing using sha256
bool hashing(std::ifstream& file, unsigned char out[EVP_MAX_MD_SIZE], unsigned int& out_len) {
   
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) return false;

  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    return false;
  }
  
  char buffer[64 * 1024];
    
  while (file) {
    file.read(buffer, sizeof(buffer));
    std::streamsize n = file.gcount();
    if (n > 0)
      EVP_DigestUpdate(ctx, buffer, n);
  }

  EVP_DigestFinal_ex(ctx, out, &out_len);
  EVP_MD_CTX_free(ctx);
  return true;
}



// converts the hash to hex-encoding so we are not storing raw binary bytes
// this is needed since we are treating it like a string while reading from the index files
std::string hash_value_to_store(const unsigned char* hash, size_t len) {

  std::ostringstream oss;
  for (size_t i = 0; i < len; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
  }
  return oss.str();
}





