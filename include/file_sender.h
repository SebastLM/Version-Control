#ifndef FILE_SENDER_H
#define FILE_SENDER_H

#include <fstream>
#include <string>

int send_file(int sock, const std::string& file_to_send, bool is_dir);

int send_remove_entry(int sock, const std::string& entry_to_remove);

#endif
