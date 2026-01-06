#ifndef INDEX_H
#define INDEX_H

#include <fstream>
#include <set>
#include <unordered_map>


struct IndexEntry {
    bool is_dir;
    std::string hash;
    bool visited = false;
};


using ProjectIndex =
    std::unordered_map<std::string, IndexEntry>;


ProjectIndex load_index(const std::string& index_path);


void update_index(ProjectIndex& index, const std::string& path, bool is_dir, const std::string& new_hash);


void remove_deleted(ProjectIndex& index, const std::set<std::string>& current_files);


void save_index(const std::string& index_path, const ProjectIndex& index, const std::string& project_root);


#endif