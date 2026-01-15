#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>


#include "file_sender.h"
#include "path_trim.h"
#include "commit.h"





namespace fs = std::filesystem;


int commit(int sock) {

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


    /*
    TODO: will need to adapt file sender so it can handle receiving  the stage file(the problem with this file is that each line is path|type|hash|add_rm)

        have aux function cleaning the input in send_files

    */
    // call file sender to handle stage file and commit files to server  
    file_sender(sock, stage_path);

    // clearing stage file after commit
    std::ofstream stage(stage_path, std::ios::trunc);
    if (!stage.is_open()) {
        std::cout << "failed to open file for cleaning" << std::endl;
        std::cout << "consider manualy cleaning: " + stage_path << std::endl;
        return 1;
    }
    stage.close();








    /*
    TODO: later handle sending messages
    */
}
