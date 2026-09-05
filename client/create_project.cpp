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

  using ProjectIndex = std::unordered_map<std::string, IndexEntry>;

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

      std::string existing_index =
          new_project_root_tmp.generic_string() + "/.index";
      if (fs::exists(existing_index)) {
        std::cerr << "WARNING: The directory you're trying to create a project "
                     "in already has one.\n"
                  << "Manually remove the .index file if you want to create a "
                     "new project in that same directory.\n";
        return -1;
      }
    }

    new_index_path = new_project_root + "/.index";

    save_index(new_index_path, new_project_index, new_project_root);

  } catch (const std::exception &e) {

    if (!dot)
      fs::remove_all(new_project_root_tmp);

    std::cerr << "Failed  to create_project: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
