#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>


#include "file_sender.h"
#include "path_trim.h"
#include "staging.h"
#include "protocol.h"
#include "send_all_recv_all.h"


namespace fs = std::filesystem;



void commit_action(std::string stage_path, std::unordered_map<std::string, Action>& commit_plan) {

  std::ifstream stage(stage_path, std::ios::binary);
  if (!stage.is_open()) {
      perror("failed to open stage file");
      return;
  }

  std::string line;
  while (std::getline(stage, line)) {
  
    if (line.empty())
      continue;

    // path|type|hash|add_rm
    auto p1 = line.find('|');
    auto p2 = line.find('|', p1 + 1);
    auto p3 = line.find('|', p2 + 1);

    if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) {
      continue; // Skip malformed lines
    }

    std::string path = line.substr(0, p1);
    std::string type = line.substr(p1 + 1, p2 - p1 - 1);
    std::string hash = line.substr(p2 + 1, p3 - p2 - 1);
    std::string add_rm = line.substr(p3 + 1);

    Action a = Action::Remove;
    if (add_rm == "1") {
      if (type == "dir")
        a = Action::AddDir;
      else
        a = Action::AddFile;
    }

    std::string path_only = path;
    obtain_path(path_only);

    if (commit_plan[path_only] != Action::Remove)
      commit_plan[path] = a;
  }
  
  stage.close();
}




int commit_remote(std::unordered_map<std::string, Action> commit_plan, int sock) {

  int line_file = 0;
  std::string desync_path;
  // TODO: too handle the desync logic, the remote needs to return what it got
  // this way we can verify that everything went acordingly
  // in case of the actual file sending, we just need to confirm if the hash matched on the other side( if the remote returns "OK" we just accept it)
  //IMP TODO: now in the remote so we dont just send everything again, we need to start sending a flag from here saying which operation is being executed
  Action desync_action;  

  for (const auto& [path, action] : commit_plan) {
    
    // no need to worry about endianess since its a 1 byte stream
    send_all(sock, &action, sizeof(action));
    line_file++;
    
    int result = 0;
    switch (action) {
      case Action::Remove:
        result = send_remove_entry(sock, path);
        break;
      
      case Action::AddFile:
        result = send_file(sock, path, 0);
        break;
      
      case Action::AddDir:
        result = send_file(sock, path, 1);
        break;

      default:
        std::cerr << "Undefined behaviour, error in line " << line_file << "of stage file: " << path << std::endl 
                  << "consider manualy fixing the line" << std::endl;
        break;
    }

    if (result != 0) {
      /*
        TODO: handle file desync logic here... IMP
      */
      std::cerr << "WARNING: Failed to process " << path << "at line " << line_file << std::endl;
    }
  }
}






int commit(int sock) {
    
  std::unordered_map<std::string, Action> commit_plan;

  // obtain the current working dir from where the executable is called
  fs::path c_work_dir = fs::current_path();
  std::string project_root_path = c_work_dir.generic_string();
  
  std::string stage_path = project_root_path + "/.stage";
  
  const char* stage_path_tmp = stage_path.c_str();
  int res = access(stage_path_tmp, R_OK);
  if (res < 0) {
    if (errno == ENOENT) {
      std::cerr << "project doesnt exist: No such file or directory" << std::endl;
      return EXIT_FAILURE;
    }
  }

  commit_action(stage_path, commit_plan);

  commit_remote(commit_plan, sock);
  
  Action end_signal = Action::EndCommit;
  if (send_all(sock, &end_signal, sizeof(end_signal)) < 0) {
    std::cerr << "Failed to send EndCommit signal" << std::endl;
    return 1;
  }
  std::cout << "\nCommit finished." << std::endl;

  // clearing stage file after commit
  std::ofstream file(stage_path, std::ios::trunc);
  if (!file.is_open()) {
      std::cout << "WARNING: failed to open file for cleaning commit stage" << std::endl;
      return -1;
  }
  file.close();

  return 0;
}

