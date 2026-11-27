#include "../../src/system/sd_write_file.h"
#include <cassert>
#include <filesystem>
// ESP-IDF 4.4.3 FAT VFS does not register fcntl. The pinned Newlib fdopen
// calls F_GETFL and fails; ordinary open/read/write/fsync/close remain valid.
extern "C" FILE* __wrap_fdopen(int,const char*) { errno=ENOSYS; return nullptr; }
static bool rejectSync=false;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    if (rejectSync) { errno=EIO; return -1; }
    return __real_fsync(fd);
}
int main() {
    char directory[]="/tmp/meowkit-fat-vfs-XXXXXX"; assert(mkdtemp(directory));
    meow_storage::WriteFile file;
    assert(file.open(directory,"test.ir"));
    const uint8_t text[]="Filetype: IR signals file\nVersion: 1\n";
    assert(file.write(text,sizeof(text))==sizeof(text) && file.sync() && file.close());
    char error[96]{}; assert(meow_storage::probe(directory,"probe.tmp",error,sizeof(error)));
    rejectSync=true;
    assert(file.open(directory,"failed-sync.ir"));
    assert(file.write(text,sizeof(text))==sizeof(text));
    assert(!file.sync() && file.errorCode()==EIO && !strcmp(file.operation(),"sync"));
    assert(file.close());
    assert(!meow_storage::probe(directory,"failed-probe.tmp",error,sizeof(error)));
    assert(strstr(error,"sync:") && !std::filesystem::exists(std::string(directory)+"/failed-probe.tmp"));
    rejectSync=false;
    std::filesystem::remove_all(directory);
    puts("FAT VFS checks passed (writes and sync work without fdopen/fcntl)");
}
