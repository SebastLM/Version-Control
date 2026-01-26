#ifndef FILE_SENDER_H
#define FILE_SENDER_H

#include <fstream>

int send_file(int sock, std::string& file_to_send, bool is_dir);

int send_remove_entry(int sock, std::string& entry_to_remove);

#endif
