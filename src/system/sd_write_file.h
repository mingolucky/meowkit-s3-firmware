#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>

namespace meow_storage {
// Checked VFS writer shared by learned remotes, PCAP and the storage probe.
class WriteFile {
    int _fd=-1;
    const char* _operation="";
    int _error=0;
    char _path[256]{};
    bool failed(const char* operation) {
        if (!_error) { _error=errno?errno:EIO; _operation=operation; }
        return false;
    }
public:
    explicit operator bool() const { return _fd>=0; }
    const char* operation() const { return _operation; }
    int errorCode() const { return _error; }
    const char* path() const { return _path; }
    bool open(const char* directory,const char* name) {
        if (_fd>=0) { errno=EBUSY; return failed("open"); }
        _error=0; _operation="";
        errno=0;
        if (::mkdir(directory,0777)!=0 && errno!=EEXIST) return failed("mkdir");
        const int n=snprintf(_path,sizeof(_path),"%s/%s",directory,name);
        if (n<0 || size_t(n)>=sizeof(_path)) { errno=ENAMETOOLONG; return failed("path"); }
        errno=0;
        // fdopen requires fcntl(F_GETFL), which the pinned FAT VFS does not
        // implement. Keep the supported descriptor API all the way through.
        _fd=::open(_path,O_WRONLY|O_CREAT|O_EXCL,0666);
        if (_fd<0) return failed("create");
        return true;
    }
    size_t write(const uint8_t* data,size_t len) {
        if (_fd<0) { errno=EBADF; failed("write"); return 0; }
        size_t written=0;
        while (written<len) {
            errno=0; const auto n=::write(_fd,data+written,len-written);
            if (n<0 && errno==EINTR) continue;
            if (n<=0) { failed("write"); break; }
            written+=size_t(n);
        }
        return written;
    }
    bool sync() {
        if (_fd<0) { errno=EBADF; return failed("sync"); }
        if (_error) return false;
        errno=0; if (fsync(_fd)!=0) return failed("sync");
        return true;
    }
    bool close() {
        if (_fd<0) return true;
        const int fd=_fd; _fd=-1;
        errno=0; if (::close(fd)!=0) return failed("close");
        return true;
    }
};

inline bool probe(const char* root,const char* name,char* error,size_t capacity) {
    WriteFile file;
    if (!file.open(root,name)) {
        snprintf(error,capacity,"%s: %s (%d)",file.operation(),strerror(file.errorCode()),file.errorCode());
        return false;
    }
    static const uint8_t expected[]={0x4d,0x65,0x6f,0x77,0,0xff,0x55,0xaa};
    bool ok=file.write(expected,sizeof(expected))==sizeof(expected) && file.sync();
    if (!file.close()) ok=false;
    if (!ok) snprintf(error,capacity,"%s: %s (%d)",file.operation(),strerror(file.errorCode()),file.errorCode());
    if (ok) {
        FILE* read=fopen(file.path(),"rb"); uint8_t found[sizeof(expected)]{};
        ok=read && fread(found,1,sizeof(found),read)==sizeof(found) && !memcmp(found,expected,sizeof(found));
        if (read) fclose(read);
        if (!ok) snprintf(error,capacity,"Read-back verification failed");
    }
    if (remove(file.path())!=0 && ok) { snprintf(error,capacity,"Probe cleanup: %s",strerror(errno)); ok=false; }
    return ok;
}
}
