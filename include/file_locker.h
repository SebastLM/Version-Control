#ifndef FILE_LOCKER_H
#define FILE_LOCKER_H

#include <string>
#include <shared_mutex>


// preventing external process from accessing the file while we do it
class SystemWideLock {
  private:
#ifdef _WIN32
    void* hFile;
#else
    int fd;
#endif

  public:
    SystemWideLock();
    ~SystemWideLock();
    bool acquire(const std::string& path, bool write);
    void release();
};


// going to be used in the server when we implement multithreading to support multiple "clients"
class ThreadLock {
  private:
    std::shared_mutex mutex;

  public:
    void lock_read();
    void unlock_read();
    void lock_write();
    void unlock_write();
};


// locks both cases
class GuardLock {
  private:
    SystemWideLock& sys;
    ThreadLock& thr;
    std::string path;
    bool writing;

  public:
    GuardLock(SystemWideLock& s, ThreadLock& t, const std::string& p, bool write);
    ~GuardLock();
};

#endif
