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


int commit(int sock) {
    
    std::unordered_map<std::string, Action> commit_plan;

    /*
        TODO: for now the commits only work if done from the base project dir, change that
    */

    // obtain the current working dir from where the executable is called
    fs::path c_work_dir = fs::current_path();
    std::string project_name = c_work_dir.generic_string();
    file_name(project_name);

    std::string stage_path = "/active_projects/" + project_name + "/stage";
    
    const char* stage_path_tmp = stage_path.c_str();
    int res = access(stage_path_tmp, R_OK);
    if (res < 0) {
        // in case the file doesnt exist
        if (errno == ENOENT) {
            perror("project doesnt exist");
            return EXIT_FAILURE;
    }

    std::ifstream stage(stage_path, std::ios::binary);
    if (!stage.is_open()) {
        perror("failed to open stage file");
        return EXIT_FAILURE;
    }

    std::string line;
    while (std::getline(stage, line)) {
        if (line.empty())
            continue;

        // path|type|hash
        auto p1 = line.find('|');
        auto p2 = line.find('|', p1 + 1);
        auto p3;

        std::string path = line.substr(0, p1);
        std::string type = line.substr(p1 + 1, p2 - p1 - 1);
        std::string add_rm = line.substr(p2 + 1);

        
        Action a = Action::Remove;
        if (add_rm == "1") {
            if (type == "dir")
                a = Action::AddDir;
            else
                a = Action::AddFile;
        }

        std::string path_only = path;
        obtain_path(path_only);


        /*
        TODO: still need to handle better the file we are trying to commit for removal being part of an already removed(in the server side) dir
        */
        if (commit_plan[path_only] != Action::Remove)
          commit_plan[path] = a;
    }

    int line_file = 0;
    for (const auto& [path, action] : commit_plan) {
      
      // no need to worry about endianess since its a 1 byte stream
      send_all(sock, &action, sizeof(action));
      line_file++;
      
      switch (action) {
        case Action::Remove:
          send_remove_entry(sock, path);
          break;
        
        case Action::AddFile:
          send_file(sock, path, 0);
          break;
        
        case Action::AddDir:
          send_file(sock, path, 1);
          break;

        default:
          std::cout << "Undefined behaviour, error in line " << line_file << "of stage file: " << path << std::endl 
                    << "consider manualy fixing the line" << std::endl;
          break;
        }
        
    }



    // pass the socket set up here and create the auxiliar functions in file_sender for the different actions, like remove, ... 
    // call file sender to handle stage file and commit files to server  

    // clearing stage file after commit
    std::ofstream stage(stage_path, std::ios::trunc);
    if (!stage.is_open()) {
        std::cout << "failed to open file for cleaning commit stage" << std::endl;
        std::cout << "consider manualy cleaning to avoid further errors: " + stage_path << std::endl;
        return 1;
    }
    stage.close();








    /*
    TODO: later handle sending messages
    */
}
