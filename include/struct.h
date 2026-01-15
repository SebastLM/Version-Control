#ifndef STRUCT_H
#define STRUCT_H

#include <string>


// used to store index entrys
// path is stored as the key of the unordered_map where this is used
struct IndexEntry {
    bool is_dir;
    std::string hash;
    bool visited = false;
};



#endif