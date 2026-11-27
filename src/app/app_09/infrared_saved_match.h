#pragma once
#include "infrared_signal.h"
#include <algorithm>
#include <utility>

namespace meow_ir {
struct SavedCommandMatch {
    std::string path,remote,button;
};

// Validate a complete remote before publishing any matches from it. A corrupt
// later record must not lend an apparently valid name to an earlier record.
class SavedMatchFile {
    const Signal& query;
    std::string path,type,version;
    RecordParser parser;
    bool begun=false,bad=false;
    size_t buttons=0,limit;
    std::vector<SavedCommandMatch> found;
    void record() {
        if(!begun || bad) return;
        if(!parser.valid() || ++buttons>MaxButtons) { bad=true; return; }
        const auto& signal=parser.signal;
        const auto add=[&](const char* part) {
            if(found.size()>=limit) { limited=true; return; }
            auto remote=path.substr(path.find_last_of('/')+1);
            remote.resize(remote.size()-3); // scanner only opens .ir files
            found.push_back({path,remote,signal.name+part});
        };
        if(samePayload(query,signal)) add(signal.powerOff?" [On]":"");
        if(signal.powerOff && samePayload(query,*signal.powerOff)) add(" [Off]");
    }
public:
    bool limited=false;
    SavedMatchFile(const Signal& signal,std::string file,size_t maxMatches=64)
        :query(signal),path(std::move(file)),limit(maxMatches) {}
    bool invalid() const { return bad; }
    void line(std::string text) {
        if(bad) return;
        if(text.compare(0,3,"\xEF\xBB\xBF")==0) text.erase(0,3);
        const auto trim=[](std::string& s) {
            const auto first=s.find_first_not_of(" \t\r\n");
            if(first==std::string::npos) { s.clear(); return; }
            s=s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
        };
        trim(text); if(text.empty() || text[0]=='#') return;
        const auto colon=text.find(':'); if(colon==std::string::npos || !colon) return;
        auto key=text.substr(0,colon),value=text.substr(colon+1); trim(key); trim(value);
        if(key=="name") {
            if(!begun && (type!="IR signals file" || version!="1")) { bad=true; return; }
            record(); if(bad) return;
            parser={}; begun=true;
        }
        if(begun) { parser.field(key,value); if(!parser.ok) bad=true; }
        else if(key=="Filetype") type=value;
        else if(key=="Version") version=value;
    }
    bool finish() {
        record(); return !bad && type=="IR signals file" && version=="1";
    }
    const std::vector<SavedCommandMatch>& matches() const { return found; }
};

// One step performs at most one 512-byte read or directory operation. The UI
// supplies current millis for the search deadline and can cancel on transitions. Only one
// directory and one remote remain open, respecting Arduino's five-file limit.
template<class FS> class SavedCommandSearch {
    using Handle=decltype(std::declval<FS&>().open("/"));
    struct Folder { std::string path; size_t depth; };
    FS& fs;
    Signal query;
    Handle directory,file;
    Folder current;
    std::vector<Folder> folders;
    std::unique_ptr<SavedMatchFile> parser;
    std::string line;
    bool active=false;
    size_t files=0;
    size_t fileBytes=0,fileSize=0;
    size_t visitedEntries=0,visitedFolders=0,totalBytes=0;
    uint32_t started=0;
    static constexpr size_t MaxEntries=4096,MaxFolders=128,MaxTotalBytes=4*1024*1024;
    static constexpr uint32_t MaxMilliseconds=3000;
    static std::string lower(std::string value) {
        for(auto& c:value) if(c>='A' && c<='Z') c=char(c-'A'+'a');
        return value;
    }
    void endFile(bool valid) {
        if(valid && parser->finish()) {
            for(const auto& match:parser->matches()) {
                if(matches.size()<64) matches.push_back(match); else limited=true;
            }
            limited|=parser->limited;
        } else ++unreadable;
        file.close(); parser.reset(); line.clear();
    }
public:
    std::vector<SavedCommandMatch> matches;
    size_t unreadable=0;
    bool limited=false;
    explicit SavedCommandSearch(FS& source):fs(source) {}
    bool running() const { return active; }
    bool incomplete() const { return limited || unreadable>0; }
    void cancel() {
        if(active) limited=true;
        active=false; file.close(); directory.close(); parser.reset();
        folders.clear(); line.clear(); query={};
    }
    void clear() {
        cancel(); matches.clear(); unreadable=0; limited=false; files=0;
        fileBytes=fileSize=visitedEntries=visitedFolders=totalBytes=0; started=0;
    }
    void begin(const Signal& signal,uint32_t now=0) {
        clear(); query=signal; started=now;
        if(!fs.exists("/infrared")) return; // no saved remotes yet
        active=true; visitedFolders=1; folders.push_back({"/infrared",0});
    }
    void step(uint32_t now=0) {
        if(!active) return;
        // Both default arguments leave elapsed time at zero for legacy callers.
        // Unsigned subtraction enforces the deadline even when millis wraps to zero.
        if(uint32_t(now-started)>=MaxMilliseconds) { cancel(); return; }
        if(file) {
            uint8_t bytes[512];
            const size_t remaining=MaxTotalBytes-totalBytes;
            if(!remaining) {
                // A file ending exactly at the byte budget is complete only if
                // its size still agrees. Never publish a budget-truncated file.
                if(fileBytes!=fileSize || file.size()!=fileSize) { cancel(); return; }
                if(!line.empty()) parser->line(line);
                endFile(!parser->invalid()); return;
            }
            const size_t requested=std::min(sizeof(bytes),remaining);
            const auto count=file.read(bytes,requested);
            if(size_t(count)>requested) { endFile(false); return; }
            if(!count) {
                if(fileBytes<fileSize) { endFile(false); return; }
                if(!line.empty()) parser->line(line);
                endFile(!parser->invalid()); return;
            }
            fileBytes+=size_t(count); totalBytes+=size_t(count);
            for(size_t i=0;i<size_t(count);++i) {
                if(bytes[i]=='\n') { parser->line(std::move(line)); line.clear(); }
                else if(line.size()<MaxTimings*9+128) line+=char(bytes[i]);
                else { endFile(false); return; }
                if(parser->invalid()) { endFile(false); return; }
            }
            return;
        }
        if(directory) {
            if(visitedEntries>=MaxEntries) { cancel(); return; }
            auto entry=directory.openNextFile();
            if(!entry) { directory.close(); return; }
            ++visitedEntries; // Include hidden and unrelated entries in the work budget.
            std::string name=entry.name(); name=name.substr(name.find_last_of('/')+1);
            if(name.empty() || name[0]=='.') { entry.close(); return; }
            const auto folded=lower(name);
            const auto path=current.path+"/"+name;
            if(entry.isDirectory()) {
                entry.close();
                if(current.depth==0 && (folded=="universal" || folded=="assets")) return;
                if(current.depth>=12 || visitedFolders>=MaxFolders || path.size()>220) { cancel(); return; }
                ++visitedFolders;
                folders.push_back({path,current.depth+1}); return;
            }
            entry.close();
            if(folded.size()<4 || folded.substr(folded.size()-3)!=".ir") return;
            if(path.size()>220) { cancel(); return; }
            if(++files>2048) { cancel(); return; }
            file=fs.open(path.c_str());
            if(!file || file.isDirectory()) {
                file.close(); ++unreadable; return;
            }
            fileBytes=0; fileSize=file.size();
            if(fileSize>16*1024*1024) { file.close(); ++unreadable; return; }
            if(fileSize>MaxTotalBytes-totalBytes) { cancel(); return; }
            parser=std::make_unique<SavedMatchFile>(query,path); return;
        }
        if(folders.empty()) {
            auto root=fs.open("/infrared");
            if(!root || !root.isDirectory()) ++unreadable;
            root.close(); active=false; query={}; return;
        }
        current=std::move(folders.back()); folders.pop_back();
        directory=fs.open(current.path.c_str());
        if(!directory || !directory.isDirectory()) { directory.close(); ++unreadable; }
    }
};
} // namespace meow_ir
