#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>


#include "protocol.h"
#include "hash.h"
#include "path_trim.h"
#include "index.h"  


namespace fs = std::filesystem;


/*
    The commit has of now will do the following:
        1. obtain the place where the executable is called
        2. read from the file that stores the state(tree, hash) of the project(active_projects/project/index)
        3. verify accordingly to its previous registered project_tree and file_hashes if anything has been changed, created or deleted
        4. send the changed files to the host and keep the new record of information(tree, file hashes)
*/
int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "error, usage should be: ./(...) file1 file2 ..." << std::endl;
        std::cout << "You could also use:     ./(...) ." << std::endl;
        return EXIT_FAILURE;
    }

    using ProjectIndex =
    std::unordered_map<std::string, IndexEntry>;

    /*
        TODO: for now the commits only work if done from the base project dir, change that 
    */

    // obtain the current working dir from where the executable is called
    fs::path c_work_dir = fs::current_path();
    std::string project_name = c_work_dir.generic_string();
    file_name(project_name);

    // load index
    std::string index_path = "/active_projects/" + project_name + "/index";
    ProjectIndex old_index = load_index(index_path);

    ProjectIndex new_index;
    // creatin the index of the current project tree
    for (const auto& dirEntry : fs::recursive_directory_iterator(c_work_dir)) {
        // std::cout << dirEntry << std::endl;
        IndexEntry e;
        e.is_dir = false;

        if (dirEntry.is_directory()) {
            e.is_dir = true;
            e.hash = "";
        } else {

            std::ifstream file(dirEntry.path().string(), std::ios::binary);
            unsigned char out[32];
            unsigned int out_len;

            if (hashing(file, out, out_len)) 
                e.hash = hash_value_to_store(out, out_len);
        }
        auto it = old_index.find(dirEntry.path());

        // if the path exist in the previou state as well
        if (it != old_index.end()) {
            it->second.visited = true;
        }

        new_index[dirEntry.path().string()] = e;
    }

    for (const auto& [path, entry] : new_index) {
        
        if (old_index[path].visited)
            if (old_index[path].hash != new_index[path].hash)
                /*

////////////   TODO: add the file in the commit file

                */
            continue;
        // returns iterator to path
        auto it = old_index.find(path);

        // if the returned iterator value doesnt exist, meaning the file is new
        if (it == old_index.end()) {
            /*

//////////// TODO: add the file in the commit file

            */
        }
    }

    /*
////////  TODO: need to worry about deleted files
                maybe create a file that contains the deleted files, to send over to the host
    */

}
