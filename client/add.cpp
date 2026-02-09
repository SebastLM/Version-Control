#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>

#ifdef _WIN32 
    inline int access(const char *pathname, int mode) {
        return _access(pathname, mode);
    }
#else
#include <unistd.h>
#endif


#include "protocol.h"
#include "hash.h"
#include "path_trim.h"
#include "index.h"
#include "staging.h"



namespace fs = std::filesystem;


/*
    The add has of now will do the following:
        1. obtain the place where the executable is called
        2. read from the file that stores the state(tree, hash) of the project(active_projects/project/index)
        3. verify accordingly to its previous registered project_tree and file_hashes if anything has been changed, created or deleted
        4. add the changes to the stage file
*/
int add() {

    using ProjectIndex =
    std::unordered_map<std::string, IndexEntry>;

    /*
        TODO: for now the commits only work if done from the base project dir, change that
    */

    // obtain the current working dir from where the executable is called
    fs::path c_work_dir = fs::current_path();
    std::string project_name = c_work_dir.generic_string();
    file_name(project_name);

    std::string index_path = "/active_projects/" + project_name + "/index";
    
    const char* index_path_tmp = index_path.c_str();
    int res = access(index_path_tmp, R_OK);
    if (res < 0) {
        // in case the file doesnt exist
        if (errno == ENOENT) {
            perror("project doesnt exist");
            return EXIT_FAILURE;
    }

    // load index
    ProjectIndex old_index = load_index(index_path);

    ProjectIndex new_index;
    // creating the index of the current project tree
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
            file.close();
        }
        auto it = old_index.find(dirEntry.path());

        // if the path exist in the previous state as well
        if (it != old_index.end()) {
            it->second.visited = true;
        }

        new_index[dirEntry.path().string()] = e;
    }


    // open stage file for writing
   /*
      The file will look like:
       1: # project_root=/home/sebastlm/desktop/project1
       2: file_path|file_type|hash|add_rm
       3: ...
       ...

       the add_rem value will be:
          - 0 in case remove
          - 1 in case add

      <this will allow me to distinguish the deleted files in the end, making sure that the server logic is easier>
   
   */
    std::string stage_path = "/active_projects/" + project_name + "/stage";

    // allows adding multiple times
    // handling repeated entries on the file sender it self
    std::ofstream stage(stage_path, std::ios::app); 

    if (!stage.is_open()) {
        perror("failed to open Files to commit\n");
        return EXIT_FAILURE;
    }

    for (const auto& [path, entry] : new_index) {
        
        if (old_index[path].visited) {
            if (old_index[path].hash != new_index[path].hash) {

                // add the commit object to the vector in case of a modified file
                std::string type = "file";
                if (new_index[path].is_dir)
                    type = "dir";

                std::string stage_file = path + "|" + type + "|" + new_index[path].hash + "|1";
                stage << stage_file << "\n";
            }
            continue;
        }
        // returns iterator to path
        auto it = old_index.find(path);

        // if the returned iterator value doesnt exist, meaning we have a new entry in the project(file/dir)
        if (it == old_index.end()) {

            // add the commit object to stage for it to be commited later
            std::string type = "file";
            if (new_index[path].is_dir)
                type = "dir";

            std::string stage_file = path + "|" + type + "|" + new_index[path].hash + "|1";
            stage << stage_file << "\n";
        }
    }

    // file to be deleted
    for (const auto& [path, entry] : old_index) {
      if (!old_index[path].visited) {
      
        std::string type = "file";
        if (new_index[path].is_dir)
            type = "dir";

        std::string stage_file = path + "|" + type + "|0";
        stage << stage_file << "\n";
      }
    }

    stage.close();
    // saving updated index file
    save_index(index_path, new_index, project_name);
    
    }
  return 0;
}
