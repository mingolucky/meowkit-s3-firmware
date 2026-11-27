#pragma once
#include "sd_write_file.h"
#include <cstdlib>
#include <dirent.h>
#include <limits>
#include <new>
#include <initializer_list>
namespace meow_storage {
inline uint32_t recoveryCRC32(const uint8_t* data,size_t n) {
 uint32_t crc=0xffffffff;while(n--){crc^=*data++;for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&uint32_t(-int32_t(crc&1)));}return ~crc;
}
// 0 absent, 1 exact verified identity, -1 unreadable or unexpected marker.
inline int readRecoveryMarker(const char* root,const char* name,const char* identity) {
 char path[512];const int len=snprintf(path,sizeof(path),"%s/%s",root,name);
 if(len<0 || size_t(len)>=sizeof(path))return -1;
 errno=0;FILE* f=fopen(path,"rb");if(!f)return errno==ENOENT?0:-1;
 char data[128];const size_t n=fread(data,1,sizeof(data),f);
 bool ok=!ferror(f) && n==strlen(identity) && !memcmp(data,identity,n);
 if(fclose(f))ok=false;
 return ok?1:-1;
}
inline bool writeRecoveryMarker(const char* root,const char* name,const char* identity) {
 WriteFile file;if(!file.open(root,name))return false;
 bool ok=file.write(reinterpret_cast<const uint8_t*>(identity),strlen(identity))==strlen(identity) && file.sync();
 if(!file.close())ok=false;
 return ok && readRecoveryMarker(root,name,identity)==1;
}

inline bool knownRecoveryPath(const char* path) {
 for(const char* folder:{"/infrared","/handshakes","/nfc"}) {
  const size_t n=strlen(folder);if(!strncmp(path,folder,n) && (!path[n] || path[n]=='/'))return true;
 }
 return false;
}
template<class Emit> bool inventoryLegacyWalk(const char* root,const char* path,Emit& output,size_t& budget,unsigned depth) {
 if(depth>8)return false;
 char absolute[512];const int n=snprintf(absolute,sizeof(absolute),"%s%s",root,path);
 if(n<0 || size_t(n)>=sizeof(absolute))return false;
 struct stat st{};if(stat(absolute,&st) || st.st_size<0)return false;
 const bool directory=S_ISDIR(st.st_mode);
 if(*path) {if(!budget)return false;--budget;output(path,directory,directory?0:uint64_t(st.st_size));}
 if(!directory)return S_ISREG(st.st_mode);
 DIR* dir=opendir(absolute);if(!dir)return false;bool ok=true;
 for(;;) {
  errno=0;auto* item=readdir(dir);if(!item){if(errno)ok=false;break;}
  if(!strcmp(item->d_name,".") || !strcmp(item->d_name,".."))continue;
  if(strchr(item->d_name,'/') || strchr(item->d_name,'\\')){ok=false;break;}
  char child[256];const int c=snprintf(child,sizeof(child),"%s/%s",path,item->d_name);
  if(c<0 || size_t(c)>=sizeof(child) || !inventoryLegacyWalk(root,child,output,budget,depth+1)){ok=false;break;}
 }
 if(closedir(dir))ok=false;
 return ok;
}
template<class Emit> bool inventoryLegacy(const char* root,Emit output,size_t entries=4096) {
 return inventoryLegacyWalk(root,"",output,entries,0);
}
// Known MeowKit folders are captured, including empty directories. The fixed arena
// bounds paths, metadata and data together. Failure never permits restore.
class RecoverySnapshot {
 struct Entry { Entry* next; size_t size; bool directory; char path[256]; };
 uint8_t* _arena=nullptr; size_t _capacity,_used=0; Entry* _head=nullptr; Entry* _tail=nullptr;
 bool _complete=false; unsigned _files=0;
 bool fullPath(char* out,size_t size,const char* root,const char* path) const {
  const int n=snprintf(out,size,"%s%s",root,path);return n>=0 && size_t(n)<size;
 }
 Entry* add(const char* path,size_t size,bool directory) {
  const size_t alignment=alignof(Entry),aligned=(_used+alignment-1)&~(alignment-1);
  if(strlen(path)>=sizeof(Entry::path) || aligned>_capacity || sizeof(Entry)>_capacity-aligned || size>_capacity-aligned-sizeof(Entry))return nullptr;
  auto* e=new (_arena+aligned) Entry{};e->size=size;e->directory=directory;strcpy(e->path,path);
  if(_tail)_tail->next=e;else _head=e;_tail=e;_used=aligned+sizeof(Entry)+size;return e;
 }
 bool walk(const char* root,const char* path,unsigned depth) {
  if(depth>8)return false;
  char absolute[512];if(!fullPath(absolute,sizeof(absolute),root,path))return false;
  struct stat st{};if(stat(absolute,&st))return false;
  if(S_ISDIR(st.st_mode)) {
   if(*path && !add(path,0,true))return false;
   DIR* d=opendir(absolute);if(!d)return false;bool ok=true;
   for(;;) {
    errno=0;auto* item=readdir(d);if(!item){if(errno)ok=false;break;}
    if(!strcmp(item->d_name,".") || !strcmp(item->d_name,".."))continue;
    if(strchr(item->d_name,'/') || strchr(item->d_name,'\\')){ok=false;break;}
    char child[256];int n=snprintf(child,sizeof(child),"%s/%s",path,item->d_name);
    if(n<0 || size_t(n)>=sizeof(child) || !walk(root,child,depth+1)){ok=false;break;}
   }
   if(closedir(d))ok=false;
   return ok;
  }
  if(!S_ISREG(st.st_mode) || st.st_size<0 || uint64_t(st.st_size)>_capacity)return false;
  Entry* e=add(path,size_t(st.st_size),false);if(!e)return false;
  FILE* f=fopen(absolute,"rb");if(!f)return false;
  bool ok=fread(e+1,1,e->size,f)==e->size;
  if(fgetc(f)!=EOF || ferror(f))ok=false;
  if(fclose(f))ok=false;
  if(ok)++_files;
  return ok;
 }
 static bool identical(const char* path,const uint8_t* expected,size_t size) {
  FILE* f=fopen(path,"rb");if(!f)return false;uint8_t buf[512];size_t offset=0;bool ok=true;
  while(offset<size){size_t n=size-offset;if(n>sizeof(buf))n=sizeof(buf);if(fread(buf,1,n,f)!=n || memcmp(buf,expected+offset,n)){ok=false;break;}offset+=n;}
  if(fgetc(f)!=EOF || ferror(f))ok=false;
  if(fclose(f))ok=false;
  return ok;
 }
public:
 explicit RecoverySnapshot(size_t capacity=2*1024*1024):_capacity(capacity) {}
 ~RecoverySnapshot(){free(_arena);}
 RecoverySnapshot(const RecoverySnapshot&)=delete;RecoverySnapshot& operator=(const RecoverySnapshot&)=delete;
 bool capture(const char* root) {
  _complete=false;_used=0;_files=0;_head=_tail=nullptr;
  if(!_arena)_arena=static_cast<uint8_t*>(malloc(_capacity));
  if(!_arena)return false;
  for(const char* folder:{"/infrared","/handshakes","/nfc"}) {
   char path[512];if(!fullPath(path,sizeof(path),root,folder))return false;
   struct stat st{};
   if(stat(path,&st)) {if(errno==ENOENT)continue;return false;}
   if(!S_ISDIR(st.st_mode) || !walk(root,folder,0))return false;
  }
  _complete=true;return true;
 }
 unsigned fileCount() const {return _files;}
 size_t memoryBytes() const {return _used;}
 template<class Emit> void emit(Emit output) const {
  if(!_complete)return;
  for(auto* e=_head;e;e=e->next)output(e->path,e->directory?nullptr:reinterpret_cast<const uint8_t*>(e+1),e->size);
 }
 bool restore(const char* root) const {
  if(!_complete)return false;
  for(auto* e=_head;e;e=e->next) {
   char destination[512];if(!fullPath(destination,sizeof(destination),root,e->path))return false;
   if(e->directory) {struct stat st{};if(mkdir(destination,0777) && (errno!=EEXIST || stat(destination,&st) || !S_ISDIR(st.st_mode)))return false;continue;}
   const auto* data=reinterpret_cast<const uint8_t*>(e+1);
   bool preserved=false;
   for(unsigned suffix=0;suffix<1000;++suffix) {
    // Keep .ir/.pcap/.nfc extensions recognizable by their app browsers.
    const char* slash=strrchr(destination,'/');const char* extension=strrchr(destination,'.');
    if(!extension || (slash && extension<slash))extension=destination+strlen(destination);
    char candidate[512];int n=suffix?snprintf(candidate,sizeof(candidate),"%.*s.legacy-%u%s",int(extension-destination),destination,suffix,extension):snprintf(candidate,sizeof(candidate),"%s",destination);
    if(n<0 || size_t(n)>=sizeof(candidate))return false;
    struct stat st{};
    if(!stat(candidate,&st)){if(S_ISREG(st.st_mode) && identical(candidate,data,e->size)){preserved=true;break;}continue;}
    if(errno!=ENOENT)return false;
    char* separator=strrchr(candidate,'/');if(!separator)return false;*separator='\0';
    WriteFile file;if(!file.open(candidate,separator+1))return false;
    bool ok=file.write(data,e->size)==e->size && file.sync();if(!file.close())ok=false;
    if(ok)ok=identical(file.path(),data,e->size);
    if(!ok){remove(file.path());return false;}
    preserved=true;break;
   }
   if(!preserved)return false;
  }
  return true;
 }
};
}
