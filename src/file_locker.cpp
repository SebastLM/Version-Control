#include "file_locker.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////

SystemWideLock::SystemWideLock() {
#ifdef _WIN32
    hFile = (void*)-1;
#else
    fd = -1;
#endif
}

// decunstroctur
SystemWideLock::~SystemWideLock() {
    release();
}

bool SystemWideLock::acquire(const std::string& path, bool exclusive) {
#ifdef _WIN32
    DWORD access = GENERIC_READ | GENERIC_WRITE;
    DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    hFile = CreateFileA(path.c_str(), access, share, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == (void*)-1) return false;

    OVERLAPPED ov = {0};
    DWORD flags = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    return LockFileEx((HANDLE)hFile, flags, 0, MAXDWORD, MAXDWORD, &ov);
#else
    fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) return false;
    return flock(fd, exclusive ? LOCK_EX : LOCK_SH) == 0; // acquire exclusive write lock or shared reader lock
#endif
}

void SystemWideLock::release() {
#ifdef _WIN32
    if (hFile != (void*)-1) {
        OVERLAPPED ov = {0};
        UnlockFileEx((HANDLE)hFile, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle((HANDLE)hFile);
        hFile = (void*)-1;
    }
#else
    if (fd != -1) {
        flock(fd, LOCK_UN); // release lock, shared or exclusive
        close(fd);
        fd = -1;
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadLock::lock_read() { 
  mutex.lock_shared(); 
}
void ThreadLock::unlock_read() { 
  mutex.unlock_shared(); 
}
void ThreadLock::lock_write() { 
  mutex.lock(); 
}
void ThreadLock::unlock_write() { 
  mutex.unlock(); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

GuardLock::GuardLock(SystemWideLock& s, ThreadLock& t, const std::string& p, bool write) 
    : sys(s), thr(t), path(p), writing(write) {
    if (writing) {
        thr.lock_write();
        sys.acquire(path, true);
    } else {
        thr.lock_read();
        sys.acquire(path, false);
    }
}
 
// decunstroctur to automatically destroy the lock after the func using it going out of scope
GuardLock::~GuardLock() {
    sys.release();
    if (writing) thr.unlock_write();
    else thr.unlock_read();
}
