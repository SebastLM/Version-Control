#include <string>

#include <path_trim.h>

#ifdef _WIN32
#define DIR_SPLITER '\\'
#else
#define DIR_SPLITER '/'
#endif


// obtain path to name separator pos
std::string::iterator obtain_pos(std::string& f_d_path) {
   
   if (f_d_path.empty())
      return f_d_path.begin();

   char c = DIR_SPLITER; 

   std::string::iterator p;
   for (p = f_d_path.end(); p != f_d_path.begin();--p) 
      if (*p == c)
         return p;

   return f_d_path.begin();
}



void file_name(std::string& f_d_path) {
   
   auto p = obtain_pos(f_d_path);
   if (p != f_d_path.begin())
      f_d_path.erase(f_d_path.begin(), p + 1);
}



void obtain_path(std::string& f_d_path) {
   
   auto p = obtain_pos(f_d_path);
   if (p != f_d_path.begin())
      f_d_path.erase(p + 1, f_d_path.end());
}