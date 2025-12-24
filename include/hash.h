#ifndef HASH_H
#define HASH_H

#include <openssl/evp.h>

// function declaration
bool hashing(std::ifstream& file, unsigned char out[EVP_MAX_MD_SIZE], unsigned int& out_len);

#endif
