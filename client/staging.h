#ifndef STAGING_H
#define STAGING_H

#include <vector>

int commit(int sock);

int add(std::vector<std::string> files);

int create_project(std::string new_project_name);

#endif
