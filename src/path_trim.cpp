#include <string>

#include <path_trim.h>

#ifdef _WIN32
#define DIR_SPLITER '\\'
#else
#define DIR_SPLITER '/'
#endif


// obtain file_name/dir_name from a path
void file_name(std::string& f_d_path) {
   
   if (f_d_path.empty())
      return;

   char c = DIR_SPLITER; 

   std::string::iterator p;
   for (p = f_d_path.end(); p != f_d_path.begin();--p) 
      if (*p == c)
         break;

   f_d_path.erase(f_d_path.begin(), p + 1);
}