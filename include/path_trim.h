#ifndef PATH_TRIM_H
#define PATH_TRIM_H

#include <string>

// Removes directory path leaving only the file/dir name
void file_name(std::string& f_d_path);

// Removes file/dir name obtaining the path
void obtain_path(std::string& f_d_path);

// verifies if a file is within a directory
bool inside_dir(const std::string& path, std::string& dir);

#endif
