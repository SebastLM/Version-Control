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

   // start at the last actual character
   
   auto p = f_d_path.end() - 1;
  
  // if that character is a slash, skip it to find the folder above
   if (f_d_path.size() > 1 && *p == c) {
       --p;
   }

   // Search backwards
   for (; p != f_d_path.begin(); --p) {
      if (*p == c)
         return p;
   }

  // check the very first character
  if (*p == c) return p;

   return f_d_path.begin();
}



void file_name(std::string& f_d_path) {
   
   auto p = obtain_pos(f_d_path);
   if (p != f_d_path.begin())
      f_d_path.erase(f_d_path.begin(), p + 1);
}



void obtain_path(std::string& f_d_path) {
   
   auto p = obtain_pos(f_d_path);
   if (*p == DIR_SPLITER)
      f_d_path.erase(p, f_d_path.end());

}



// verify if a file path is inside the dir
/*
  passing both the variables directly so i dont have to make a copy of them each time
  by passing it as const as well the compiler prefents changes to them 
*/
bool inside_dir(const std::string& path, std::string& dir) {

  size_t len = 1;
  // in case its a windows machine
  #ifdef _WIN32
  len = 2;
  #endif

  std::string spliter(len, DIR_SPLITER);

  if (dir.back() != DIR_SPLITER) dir.append(spliter);
  return path.find(dir) == 0;
}
