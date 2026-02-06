#ifndef SEND_ALL_RECV_ALL_H
#define SEND_ALL_RECV_ALL_H

#include <cstddef> // i need this for the size_t

// function used to send and intier file
int send_all(int sock, const void* data, size_t len);

// funciton to recv an intier file
int recv_all(int sock, void* data, size_t len);

#endif // SEND_ALL_RECV_ALL_H
