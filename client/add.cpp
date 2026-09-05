#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>


#include "protocol.h"
#include "hash.h"
#include "path_trim.h"
#include "index.h"
#include "staging.h"



namespace fs = std::filesystem;


using ProjectIndex =
  std::unordered_map<std::string, IndexEntry>;





// """staging"""
/*
    The staging file will look like:
      1: # project_root=/home/sebastlm/desktop/project1
      2: relative_file_path|file_type|hash|add_rm
      3: ...
      ...

      the add_rm value will be:
        - 0 in case remove
        - 1 in case add

    <this will allow me to distinguish the deleted files or new files, so the file transfer works properly>
  
*/






// find the root dir of the project
// project_root_path passed as reference so it can be altered to the actual wanted path
int find_project_index(std::string& project_root_path) {
    
  // either until .index is found or we reach the begginning of the path
  while (true) {

    std::string index_path_tmp = project_root_path + "/.index";
  
    if (fs::exists(index_path_tmp)) {
      std::cout << "Found root project under: " << project_root_path << std::endl;  
      return 0;
    }
    
    std::string previous_path = project_root_path;
    obtain_path(project_root_path);

    // if obtain_path didnt change the string, we reached the system root
    if (project_root_path == previous_path || project_root_path.empty()) {  
        std::cerr << "Aborting: Project doesn't exist. Create one first." << std::endl;
        return -1;
    }
  }
}








// used to load stage_file into vector
void load_stage_lines(std::string stage_path, std::vector<std::string>& stage_lines) {
  
  std::string line;

  if (!fs::exists(stage_path)) {
    std::ofstream stage(stage_path);
    stage.close();
  }
  
  std::ifstream stage(stage_path);
  if (!stage.is_open()) {
    perror("failed to open stage File\n");
    return;
  }
  
  // load all current lines in stage file
  while (std::getline(stage, line)) 
    stage_lines.push_back(line);

  stage.close();
}







// write to line in stage where the file_path is found
void change_stage_file_line(std::string to_change,
                            std::string type,
                            std::string file_hash,
                            std::string add_rm,
                            std::vector<std::string>& stage_lines) { 

  int line_num = 0;
  // "line_to_insert": holds where we should add the new line in the stage file if a file of the same sub-directories is found
  // in case it exists already, it simply changes the line
  // if both cases above are not verified, simply add to the end
  int line_to_insert = -1; 

  // path used to find possible insertion spots in the stage file
  std::string to_change_path = to_change;
  obtain_path(to_change_path);

  std::string new_line = to_change + "|" + type + "|" + file_hash + "|" + add_rm;

  for (std::string& line : stage_lines){
    std::string line_path = line.substr(0, line.find('|'));
    
    // Check for an exact match
    if (line_path == to_change) {
      line = new_line;
      return;
    } else if (line.rfind(to_change_path, 0) == 0) {
      line_to_insert = line_num;
    }
    line_num++;
  }

  // if file still doesnt exist and no similar subdirectorie is found, add to the end
  if (line_to_insert == -1) {
    stage_lines.push_back(new_line);
  } else {
    stage_lines.insert(stage_lines.begin() + line_to_insert + 1, new_line);
  }
}






void add_file(const std::string& project_root_path,
              std::string hash,
              const std::string& file_path, 
              ProjectIndex& old_index,
              std::string add_rm,
              std::string type,
              std::vector<std::string>& stage_lines) {

  // relative path to file found in both the stage and index files
  fs::path absolute_path = fs::absolute(file_path);
  std::string to_change = fs::relative(absolute_path, project_root_path).string();
  
  bool exists = fs::exists(absolute_path);
  if (!exists && add_rm == "1") { // if it was "0", it was supposed to be missing
    std::cout << "WARNING: File in " << file_path 
              << "doesnt exists" << std::endl;
    return;
  }
  
  bool is_dir = fs::is_directory(absolute_path);
  // hash is only empty when the function is called by the main add, and not the add_dot
  // same for type, but no need for 2 verifications
  if (hash.empty() && add_rm != "0") {
    IndexEntry e;
    e.is_dir = is_dir;

    type = "file";
    if (is_dir) {
      e.hash = "";
      type = "dir";

    } else { 
      
      // obtain new file hash
      std::ifstream file(file_path, std::ios::binary);
      if (!file.is_open()) {
        perror("failed to open File");
        return;
      }

      unsigned char out[32];
      unsigned int out_len;

      if (hashing(file, out, out_len)) 
          e.hash = hash_value_to_store(out, out_len);
      file.close();

      hash = e.hash;
    }
    // rewrite the line if found
    // else it will just create a new entry
    old_index[to_change] = e;
  }
  // replace old line if existed, else append to end
  change_stage_file_line(to_change, type, hash, add_rm, stage_lines); 
}









int add_dot(const std::string& project_root_path, 
            const std::string& where_called,
            ProjectIndex& old_index,
            std::vector<std::string>& stage_lines) { 
 
  ProjectIndex change_index;

  // creating the index of the path where_called
  for (const auto& dirEntry : fs::recursive_directory_iterator(where_called)) {
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
    auto it = old_index.find(fs::relative(dirEntry.path(), project_root_path).string());

    // if the path exist in the previous state as well
    if (it != old_index.end()) {
        it->second.visited = true;
    }

    change_index[fs::relative(dirEntry.path(), project_root_path).string()] = e;
  }
   
  // loop for handling new files or changed ones
  for (const auto& [path, entry] : change_index) {

    if (old_index[path].visited) {
      if (old_index[path].hash != change_index[path].hash) {

        // add the commit object to the vector in case of a modified file
        std::string type = "file";
        if (change_index[path].is_dir)
            type = "dir";
        
        std::string absolute_file_path = project_root_path + "/" + path;
        add_file(project_root_path, change_index[path].hash, absolute_file_path, old_index, "1", type, stage_lines);
        // change the index file
        // as well and make sure the old_index is now visited so when trying to find the deleted ones 
        old_index[path] = change_index[path];
        old_index[path].visited = true;
      }
      continue;
    }
    // returns iterator to path
    auto it = old_index.find(path);

    // if the returned iterator value doesnt exist, meaning we have a new entry in the project(file/dir)
    if (it == old_index.end()) {

      // add the commit object to stage for it to be commited later
      std::string type = "file";
      if (change_index[path].is_dir)
          type = "dir";

      std::string absolute_file_path = project_root_path + "/" + path;
      add_file(project_root_path, change_index[path].hash, absolute_file_path, old_index, "1", type, stage_lines);
      old_index[path] = change_index[path];
      old_index[path].visited = true;
    }
  }
 
  // converting to a relative path so the inside_dir works properly
  std::string where_called_relative = fs::relative(where_called, project_root_path).string();
  // loop for handling deletions
  for (auto it = old_index.begin(); it != old_index.end(); ) {
    const std::string& path = it->first;

    if (!it->second.visited && inside_dir(path, where_called_relative)) {
      std::string type = it->second.is_dir ? "dir" : "file";
        
        // Add to stage for removal
      add_file(project_root_path, it->second.hash, path, old_index, "0", type, stage_lines);  
      it = old_index.erase(it); // safe way to erase while iterating
    } else {
        ++it;
    }
  }
  return 0;
}






// write changed lines back to actual stage file
void write_back_stage(const std::string stage_path, std::vector<std::string> stage_lines) {
  
  // This prevents corruption if the program crashes mid-write
  std::string tmp_path = stage_path + ".tmp";
  std::ofstream out(tmp_path, std::ios::trunc | std::ios::binary);
  if (!out.is_open()) {
    perror("failed to open stage File\n");
    return;
  }

  for (std::string line : stage_lines) {
    out << line << "\n" ;
  }

  // renaming the file
  std::error_code ec;
  fs::rename(tmp_path, stage_path, ec);
  if (ec) {
      fs::remove(tmp_path); // Cleanup
      throw std::runtime_error("Failed to finalize index: " + ec.message());
  }
}






int add(std::vector<std::string> files_to_commit) {  
  
  // obtain the current working dir from where the executable is called
  fs::path c_work_dir = fs::current_path();
  std::string project_root_path = c_work_dir.generic_string();
  // used to mount the specific added files directories and in case of a dot add 
  std::string where_called = project_root_path;

  // find project root dir
  find_project_index(project_root_path);
  std::string index_path = project_root_path + "/.index";
  std::string stage_path =  project_root_path + "/.stage";

  // load index
  ProjectIndex old_index = load_index(index_path);

  // vector store lines in stage file
  std::vector<std::string> stage_lines;
  load_stage_lines(stage_path, stage_lines);

 

  // add or change the new specified files
  for (std::string file : files_to_commit) {
    if (file == ".") {
      add_dot(project_root_path, where_called, old_index, stage_lines);
      if (where_called == project_root_path)
        break;
    }
    else {
      // allows user to input path to file starting with slash("/") or without it
      std::string slash = "";
      if (!(file.front() == '/'))
        slash = "/";

      std::string file_path = where_called + slash + file;
      if (fs::exists(file) && inside_dir(file, project_root_path)) // in case the file is an absolute path
        file_path = file;
      // add necessary changes to both index and stage related to the file
      add_file(project_root_path, "", file_path, old_index, "1", "",stage_lines);
    }
  }
  save_index(index_path, old_index, project_root_path);
  write_back_stage(stage_path, stage_lines);

  return 0;
}

