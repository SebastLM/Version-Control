#include <fstream>
#include <set>
#include <unordered_map>

#include "index.h"





ProjectIndex load_index(const std::string& index_path) {

    ProjectIndex index;

    std::ifstream in(index_path);

    if (!in) return index; // first commit will have index empty

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        // path|type|hash
        auto p1 = line.find('|');
        auto p2 = line.find('|', p1 + 1);
        auto p3 = 3;

        std::string path = line.substr(0, p1);
        std::string type = line.substr(p1 + 1, p2 - p1 - 1);
        std::string hash = line.substr(p2 + 1);

        IndexEntry e;
        e.is_dir = (type == "dir");
        e.hash = (hash == "-" ? "" : hash);

        index[path] = e;
    }

    return index;
}



void update_index(ProjectIndex& index, const std::string& path, bool is_dir, const std::string& new_hash) {

    auto it = index.find(path);

    if (it == index.end()) {
        // new file
        index[path] = {is_dir, new_hash};
        return;
    }

    // if hash values changed
    if (!is_dir && it->second.hash != new_hash) {
        it->second.hash = new_hash; 
    }
}



void remove_deleted(ProjectIndex& index, const std::set<std::string>& current_files) {
    for (auto it = index.begin(); it != index.end(); ) {
        if (!current_files.count(it->first))
            it = index.erase(it);
        else
            ++it;
    }
}



// is called on a commit and on a pull
void save_index(const std::string& index_path, const ProjectIndex& index, const std::string& project_root) {
    
    std::string tmp = index_path + ".tmp";
    std::ofstream out(tmp, std::ios::trunc);

    if (!out)
        throw std::runtime_error("cannot write index");

    out << "# project_root=" << project_root << "\n";

    for (const auto& [path, entry] : index) {
        out << path << "|"
            << (entry.is_dir ? "dir" : "file") << "|"
            << (entry.is_dir ? "-" : entry.hash)
            << "\n";
    }

    out.close();

    std::rename(tmp.c_str(), index_path.c_str());
}
