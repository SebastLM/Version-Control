// may not work in legacy systems, not my case
#pragma once
#include <cstdint>

// every message starts with this
enum class MsgType : uint8_t {
  CREATE_PROJECT = 1,
  COMMIT_FILES = 2,
  PULL_FILES = 3,
  // leaving open more possibilities, for now want to implement this
};

enum class Action : uint8_t{
    AddFile = 1,
    AddDir = 2,
    Remove = 3
};

