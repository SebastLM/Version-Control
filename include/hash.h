#ifndef HASH_H
#define HASH_H

#include <openssl/evp.h>
#include <sstream>
#include <iomanip>


bool hashing(std::ifstream& file, unsigned char out[EVP_MAX_MD_SIZE], unsigned int& out_len);

// hex-encoding
std::string hash_value_to_store(const unsigned char* hash, size_t len);

#endif
