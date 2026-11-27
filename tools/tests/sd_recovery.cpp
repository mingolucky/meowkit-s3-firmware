#include "../../src/system/sd_recovery.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
static bool failSync=false;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd){if(failSync){errno=EIO;return -1;}return __real_fsync(fd);}
namespace fs=std::filesystem;
static void put(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<s;}
static std::string get(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 char base[]="/tmp/meowkit-recovery.XXXXXX";assert(mkdtemp(base));fs::path root(base),old=root/"old",now=root/"new";fs::create_directories(now);
 put(old/"infrared/remote/Remote.ir","AC_ON\nAC_OFF\n");put(old/"handshakes/capture.pcap",std::string("\0\xff\x55",3));put(old/"nfc/tag.nfc","tag");fs::create_directories(old/"infrared/empty");
 put(now/"infrared/remote/Remote.ir","Windows file");
 put(old/"personal.txt","personal");
 unsigned inventoryFiles=0;bool unknown=false;
 assert(meow_storage::inventoryLegacy(old.c_str(),[&](const char* path,bool directory,uint64_t size){if(!directory)++inventoryFiles;if(size && !meow_storage::knownRecoveryPath(path))unknown=true;}));
 assert(inventoryFiles==4 && unknown);
 assert(!meow_storage::inventoryLegacy(old.c_str(),[](const char*,bool,uint64_t){},1));
 meow_storage::RecoverySnapshot snapshot;
 assert(snapshot.capture(old.c_str()));assert(snapshot.fileCount()==3);
 unsigned backupCount=0;snapshot.emit([&](const char*,const uint8_t* data,size_t){if(data)++backupCount;});assert(backupCount==3);
 assert(snapshot.restore(now.c_str()));assert(get(now/"infrared/remote/Remote.ir")=="Windows file");assert(get(now/"infrared/remote/Remote.legacy-1.ir")=="AC_ON\nAC_OFF\n");assert(get(now/"handshakes/capture.pcap")==std::string("\0\xff\x55",3));assert(fs::is_directory(now/"infrared/empty"));
 assert(snapshot.restore(now.c_str()));assert(!fs::exists(now/"infrared/remote/Remote.legacy-2.ir"));
 put(old/"infrared/large.ir",std::string(2*1024*1024+1,'x'));meow_storage::RecoverySnapshot bounded;assert(!bounded.capture(old.c_str()));assert(!bounded.restore(now.c_str()));fs::remove(old/"infrared/large.ir");
 meow_storage::RecoverySnapshot tiny(16);assert(!tiny.capture(old.c_str()));assert(!tiny.restore(now.c_str()));
 fs::remove_all(now);fs::create_directories(now);put(now/"infrared","blocked");assert(!snapshot.restore(now.c_str()));assert(get(now/"infrared")=="blocked");
 fs::remove_all(now);fs::create_directories(now);failSync=true;assert(!snapshot.restore(now.c_str()));assert(!fs::exists(now/"infrared/remote/Remote.ir"));failSync=false;
 assert(meow_storage::recoveryCRC32(reinterpret_cast<const uint8_t*>("123456789"),9)==0xcbf43926);
 const char* marker="marker";const char* identity="MEOWKIT SD RECOVERY v1\npartition=1 start=16 sectors=15495152\n";
 assert(meow_storage::readRecoveryMarker(now.c_str(),marker,identity)==0);
 assert(meow_storage::writeRecoveryMarker(now.c_str(),marker,identity));
 assert(meow_storage::readRecoveryMarker(now.c_str(),marker,identity)==1);
 assert(!meow_storage::writeRecoveryMarker(now.c_str(),marker,identity));
 assert(meow_storage::readRecoveryMarker(now.c_str(),marker,"wrong")==-1);
 failSync=true;assert(!meow_storage::writeRecoveryMarker(now.c_str(),"pending",identity));failSync=false;
 fs::remove_all(root);puts("SD recovery checks passed");
}
