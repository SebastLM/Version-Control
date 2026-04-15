// may not work in legacy systems, not my case
#pragma once
#include <cstdint>


// every message starts with this
enum class MsgType : uint8_t {
  CREATE_PROJECT = 1,
  COMMIT_FILES = 2,
  PULL_FILES = 3
  // leaving open more possibilities, for now want to implement this
};

/**
  the packet Header contains an operation value
  This value will tell us, in which stage in a exhange are we.
  Its the value that is returned to the sender so it know where we are, and if it needs to resend something again.
  This value is changed locally
 */
#pragma pack(push, 1)
struct PacketHeader {
  Op operation; // the opertation we are currently in.
  uint64_t payload_size;
  uint32_t desync;
};
#pragma pack(pop)

const uint32_t DESYNC = 0x43214321; // random number that will be used to verify the data alignment

// leaving space open for different specific operations, such as file hash, ...
enum class Op : uint8_t {
  FileName = 1,
  FileContents = 2
};

enum class OpStatus : uint8_t {
  SuccessOp = 1,
  TerminateOp = 2,
  RepeatedOp = 3,
  FailedOp = 4
};

// used for telling the server the action it has to perform
enum class Action : uint8_t{
    AddFile = 1,
    AddDir = 2,
    Remove = 3,
    EndCommit = 4
};

