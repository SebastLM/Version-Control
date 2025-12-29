#include <fstream>
#include <openssl/evp.h>

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



