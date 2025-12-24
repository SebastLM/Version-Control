
#include "hash.h"
#include <string>

class Project_main_folder {
  bool changed;
  std::string project_dir;
  

  public:
    void start_project(std::string);
    void local_changed() {changed = true;}
    char* project() {return project_dir;}
    

};

void Project_main_folder::start_project(std::string dir) {
  changed = false;
  project_dir = dir;
}


class project_file: public Project_main_folder {
  unsigned char* hash_file;

  public:
    void set_hash()
    // this logic will be usefull when i try to manage more than 1 project(repository) at a time and only trying to commit the specific changes
    void validate_commit(int fd, unsigned char result[MD5_DIGEST_LENGTH]) {
      if (hashing(fd, result) != changed) changed = true;
    }
    
  
};
