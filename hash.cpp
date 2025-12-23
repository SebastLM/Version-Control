#include "hash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/md5.h>

// Print the MD5 sum as hex-digits.
/*
void print_md5_sum(unsigned char* md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
            printf("%02x",md[i]);
    }
}
*/

// Get the size of the file by its file descriptor
unsigned long get_size_by_fd(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(EXIT_FAILURE);
    return statbuf.st_size;
}


unsigned char* hashing(int file_descriptor, unsigned char result[MD5_DIGEST_LENGTH]) {
    unsigned long file_size;
    char* file_buffer;

    // file_descript = open(argv[1], O_RDONLY);
    if(file_descriptor < 0) {
      perror("invalid file descriptor value");
      exit(EXIT_FAILURE);
    }

    file_size = get_size_by_fd(file_descriptor);
    printf("file size:\t%lu\n", file_size);

    file_buffer = mmap(0, file_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
    MD5((unsigned char*) file_buffer, file_size, result);
    munmap(file_buffer, file_size); 

    // print_md5_sum(result);
  
    return result;
}

