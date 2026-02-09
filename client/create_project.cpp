#include <filesystem>
#include <iostream>
#include <string>

#include "hash.h"
#include "index.h"
#include "staging.h"



namespace fs = std::filesystem;


int create_project(std::string new_project_name) {
 
   
  fs::path new_project_root_tmp = fs::current_path();
  std::string new_project_root = new_project_root_tmp.generic_string();
  std::string new_index_path;

  using ProjectIndex = 
      std::unordered_map<std::string, IndexEntry>;
    
  ProjectIndex new_project_index;
  
  bool dot = false;
  try {
    if (new_project_name != ".") { 

      new_project_root = new_project_root + "/" + new_project_name; 
      
      if (fs::exists(new_project_root)) {
        std::cerr << "Aborting: Project directory '" << new_project_root 
                  << "' already exists. Use a different name." << std::endl;
        return -1;
      } 

      new_project_root_tmp = new_project_root;
      fs::create_directories(new_project_root_tmp);
      
      
    } else { 

      dot = true;

      for (const auto& dirEntry : fs::recursive_directory_iterator(new_project_root_tmp)) {
        
        std::string filename = dirEntry.path().filename().string();
              
        // in case its already part of a project
        if (filename.size() > 0 && filename == ".index") {
          std::cerr << "WARNING: The directory your trying to create a project, already has one attributed to it"
                    << "\n Manually remove the .index file if you want to create a new project in that same directory" 
                    << std::endl;
          return -1;
        }

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
        
        // fs::relative
        // the path doesnt stay as absolute across filesystems
        new_project_index[fs::relative(dirEntry.path(), new_project_root_tmp).string()] = e;
      } 
    }

    new_index_path = new_project_root + "/.index";

    save_index(new_index_path, new_project_index, new_project_root);

  } catch (const std::exception& e) { 

    if (!dot)
      fs::remove_all(new_project_root_tmp);
    
    std::cerr << "Failed  to create_project: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
