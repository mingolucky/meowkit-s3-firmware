#include "../../src/app/app_09/infrared_saved_match.h"
#include "../../src/app/app_09/infrared_native_learn.h"
#include "../../src/app/app_09/infrared_power.h"
#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs=std::filesystem;
struct TestFile {
    inline static size_t openCount=0,maxOpen=0,bytesRead=0,entriesVisited=0,directoriesOpened=0;
    struct State {
        fs::path path; bool directory=false,open=true;
        std::ifstream input; std::vector<fs::path> entries; size_t nextEntry=0;
        size_t readLimit=SIZE_MAX,readCount=0;
        explicit State(fs::path p):path(std::move(p)),directory(fs::is_directory(path)) {
            ++openCount; maxOpen=std::max(maxOpen,openCount);
            if(directory) {
                for(const auto& entry:fs::directory_iterator(path)) entries.push_back(entry.path());
                std::sort(entries.begin(),entries.end());
            }
            else input.open(path,std::ios::binary);
        }
        void close() { if(open) { open=false; --openCount; input.close(); entries.clear(); } }
        ~State() { close(); }
    };
    std::shared_ptr<State> state;
    TestFile()=default;
    explicit TestFile(const fs::path& path) { if(fs::exists(path)) state=std::make_shared<State>(path); }
    explicit operator bool() const { return state && state->open; }
    bool isDirectory() const { return state && state->directory; }
    const char* name() const { return state->path.c_str(); }
    size_t size() const { return state&&!state->directory?fs::file_size(state->path):0; }
    void close() { if(state) state->close(); state.reset(); }
    TestFile openNextFile() {
        if(!state || state->nextEntry==state->entries.size()) return {};
        const auto path=state->entries[state->nextEntry++]; ++entriesVisited; return TestFile(path);
    }
    size_t read(uint8_t* buffer,size_t count) {
        if(state->readCount>=state->readLimit) return 0;
        count=std::min(count,state->readLimit-state->readCount);
        state->input.read(reinterpret_cast<char*>(buffer),count);
        const auto read=size_t(state->input.gcount()); bytesRead+=read; state->readCount+=read; return read;
    }
    bool available() { return state && state->input.peek()!=std::char_traits<char>::eof(); }
};
struct TestFS {
    fs::path root; std::string denied,failedRead; size_t failAfter=0;
    fs::path resolve(const char* path) const { return root/fs::path(path).relative_path(); }
    bool exists(const char* path) const { return fs::exists(resolve(path)); }
    TestFile open(const char* path) const {
        auto result=path==denied?TestFile{}:TestFile(resolve(path));
        if(result && result.isDirectory()) ++TestFile::directoriesOpened;
        if(result && path==failedRead) result.state->readLimit=failAfter;
        return result;
    }
};
static void write(const fs::path& path,const std::string& contents) {
    fs::create_directories(path.parent_path()); std::ofstream(path)<<contents;
}
static void finish(meow_ir::SavedCommandSearch<TestFS>& search) {
    size_t steps=0;
    while(search.running()) {
        const auto before=TestFile::bytesRead; search.step();
        assert(TestFile::bytesRead-before<=512 && ++steps<10000);
    }
    assert(TestFile::openCount==0 && TestFile::maxOpen<=3);
}
int main() {
    using namespace meow_ir;
    char temp[]="/tmp/meowkit-saved-matches-XXXXXX"; assert(mkdtemp(temp));
    TestFS source{temp,{},{},0};
    Signal on,off,other,power;
    on.name="AC On"; off.name="AC Off"; other.name="Different";
    assert(learnNative(on,"COOLIX",24,0xB23F30));
    assert(learnNative(off,"COOLIX",24,0xB27BE0));
    assert(learnNative(other,"COOLIX",24,0xB23F31));
    std::string error; assert(makePowerPair(on,off,power,error)); power.name="Power";
    const std::string header="Filetype: IR signals file\nVersion: 1\n";
    write(source.root/"infrared/remote/Bedroom AC.ir",header+serialize(on)+serialize(off)+serialize(power));
    write(source.root/"infrared/legacy.IR",header+serialize(on));
    write(source.root/"infrared/remote/Other.ir",header+serialize(other));
    Signal tv=on; tv.legacyProtocol="NEC"; tv.legacyBits=32;
    write(source.root/"infrared/remote/TV.ir",header+serialize(tv));
    write(source.root/"infrared/assets/not-personal.ir",header+serialize(on));
    write(source.root/"infrared/UNIVERSAL/not-personal.ir",header+serialize(on));
    write(source.root/"infrared/remote/broken.ir",header+serialize(on)+
        "name: Broken\ntype: parsed\nencoding: native\nprotocol: COOLIX\naddress: 00 00 00 00\ncommand: 00 00 00 00\nbits: 24\n");
    SavedCommandSearch<TestFS> search(source); search.begin(on); finish(search);
    assert(search.matches.size()==3 && search.incomplete()); // broken file is reported, never used
    bool pairedOn=false,legacy=false;
    for(const auto& match:search.matches) {
        assert(match.path.find("assets")==std::string::npos && match.path.find("broken")==std::string::npos);
        if(match.button=="Power [On]") { pairedOn=true; assert(match.remote=="Bedroom AC"); }
        if(match.path=="/infrared/legacy.IR") { legacy=true; assert(match.remote=="legacy"); }
    }
    assert(pairedOn && legacy);
    fs::remove(source.root/"infrared/remote/broken.ir");
    search.begin(off); finish(search); assert(search.matches.size()==2 && !search.incomplete());
    bool pairedOff=false; for(const auto& match:search.matches) pairedOff|=match.button=="Power [Off]";
    assert(pairedOff);
    uint8_t bytes[27]{}; Signal state; state.name="State";
    assert(learnNative(state,"DAIKIN216",216,0,bytes,sizeof(bytes)));
    write(source.root/"infrared/remote/State.ir",header+serialize(state));
    search.begin(state); finish(search); assert(search.matches.size()==1 && search.matches[0].button=="State");
    state.legacyState[0]=1; search.begin(state); finish(search); assert(search.matches.empty());
    Signal raw; raw.name="Raw"; raw.raw=true; raw.timings={9000,4500,560,1690};
    write(source.root/"infrared/remote/Raw.ir",header+serialize(raw));
    search.begin(raw); finish(search); assert(search.matches.size()==1);
    raw.timings[1]=4502; search.begin(raw); finish(search); assert(search.matches.empty());
    search.begin(on); for(int i=0;i<5;++i) search.step(); search.cancel();
    assert(!search.running() && search.incomplete() && TestFile::openCount==0);
    source.denied="/infrared/remote/Bedroom AC.ir";
    search.begin(on); finish(search); assert(search.incomplete() && search.matches.size()==1);
    source.denied.clear();
    source.failedRead="/infrared/remote/Short.ir"; source.failAfter=(header+serialize(on)).size();
    write(source.resolve(source.failedRead.c_str()),header+serialize(on)+serialize(off));
    search.begin(on); finish(search); assert(search.incomplete() && search.matches.size()==3);
    fs::remove(source.resolve(source.failedRead.c_str())); source.failedRead.clear();
    write(source.root/"infrared/upstairs/Bedroom AC.ir",std::string("\xEF\xBB\xBF")+header+serialize(on));
    search.begin(on); finish(search); assert(search.matches.size()==4 && !search.incomplete());
    bool upstairs=false,downstairs=false;
    for(const auto& match:search.matches) {
        upstairs|=match.path=="/infrared/upstairs/Bedroom AC.ir";
        downstairs|=match.path=="/infrared/remote/Bedroom AC.ir";
    }
    assert(upstairs && downstairs);
    std::string many=header; for(int i=0;i<70;++i) { on.name="Button "+std::to_string(i); many+=serialize(on); }
    write(source.root/"infrared/remote/Many.ir",many);
    search.begin(on); finish(search); assert(search.matches.size()==64 && search.limited);
    // An unrelated-file flood must stop without discarding earlier full-file matches.
    const auto learnedBefore=serialize(on);
    fs::remove_all(source.root/"infrared");
    write(source.root/"infrared/000.ir",header+serialize(on));
    for(unsigned i=0;i<4096;++i) write(source.root/("infrared/ignored-"+std::to_string(i)+".txt"),"");
    TestFile::entriesVisited=0;
    search.begin(on); finish(search);
    assert(search.limited && search.incomplete() && search.matches.size()==1);
    assert(TestFile::entriesVisited<=4096 && serialize(on)==learnedBefore);

    // A small frontier must obey a cumulative directory bound, not merely queue size.
    fs::remove_all(source.root/"infrared");
    write(source.root/"infrared/000.ir",header+serialize(on));
    for(unsigned i=0;i<20;++i) for(unsigned j=0;j<10;++j)
        fs::create_directories(source.root/("infrared/dir-"+std::to_string(i)+"/leaf-"+std::to_string(j)));
    TestFile::directoriesOpened=0;
    search.begin(on); finish(search);
    assert(search.limited && search.matches.size()==1 && TestFile::directoriesOpened<=128);

    // Multiple individually allowed files must share one aggregate byte budget.
    fs::remove_all(source.root/"infrared");
    write(source.root/"infrared/000.ir",header+serialize(on));
    std::string padding=header;
    while(padding.size()<2200*1024) padding+="# padding padding padding padding\n";
    write(source.root/"infrared/100.ir",padding);
    write(source.root/"infrared/200.ir",padding);
    const auto beforeBytes=TestFile::bytesRead;
    search.begin(on); finish(search);
    assert(search.limited && search.incomplete() && search.matches.size()==1);
    assert(TestFile::bytesRead-beforeBytes<=4*1024*1024 && serialize(on)==learnedBefore);

    // A deadline while another file is open preserves the caller and prior matches.
    search.begin(on,100);
    while(search.matches.empty()) { assert(search.running()); search.step(100); }
    while(TestFile::openCount<2) { assert(search.running()); search.step(100); }
    const auto beforeDeadline=TestFile::bytesRead;
    search.step(3099); assert(search.running());
    search.step(3100);
    assert(!search.running() && search.limited && search.incomplete());
    assert(TestFile::openCount==0 && search.matches.size()==1 && serialize(on)==learnedBefore);
    assert(TestFile::bytesRead-beforeDeadline<=512);
    search.step(4000); assert(search.matches.size()==1 && serialize(on)==learnedBefore);
    search.begin(on,UINT32_MAX-999); search.step(2000);
    assert(!search.running() && search.limited && TestFile::openCount==0 && serialize(on)==learnedBefore);
    search.begin(on,UINT32_MAX-2999); search.step(0);
    assert(!search.running() && search.limited && TestFile::openCount==0 && serialize(on)==learnedBefore);

    // Starting another query resets all aggregate limits and the prior deadline.
    fs::remove_all(source.root/"infrared");
    write(source.root/"infrared/000.ir",header+serialize(on));
    search.begin(on,5000);
    while(search.running()) search.step(5000);
    assert(search.matches.size()==1 && !search.incomplete() && TestFile::openCount==0);
    fs::remove_all(source.root);
    search.begin(on); finish(search); assert(search.matches.empty() && !search.incomplete());
    puts("Saved command matches passed (labels, paired sides, exact payloads, malformed files, cancellation, bounds and file handles)");
}
