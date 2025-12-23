#ifndef HASH_H
#define HASH_H

#include <openssl/md5.h>

// function declaration
unsigned char* hashing(int file_descriptor, unsigned char result[MD5_DIGEST_LENGTH]));

#endif
