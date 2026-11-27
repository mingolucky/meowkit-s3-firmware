#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
class String: public std::string {
public:
    using std::string::string;
    bool startsWith(const char* prefix) const { return rfind(prefix,0)==0; }
};
class File {
    struct State { std::vector<char> bytes; size_t position=0; bool directory=false; };
    std::shared_ptr<State> _state;
public:
    File()=default;
    explicit File(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return;
        _state=std::make_shared<State>(); _state->directory=std::filesystem::is_directory(path);
        if (!_state->directory) {
            std::ifstream file(path,std::ios::binary);
            _state->bytes.assign(std::istreambuf_iterator<char>(file),{});
        }
    }
    explicit operator bool() const { return bool(_state); }
    bool isDirectory() { return _state && _state->directory; }
    size_t position() const { return _state?_state->position:0; }
    size_t size() const { return _state?_state->bytes.size():0; }
    int available() { return int(size()-position()); }
    bool seek(size_t p) { if (!_state || p>size()) return false; _state->position=p; return true; }
    String readStringUntil(char separator) {
        String line;
        while (available()) { char c=_state->bytes[_state->position++]; if (c==separator) break; line+=c; }
        return line;
    }
    void close() { _state.reset(); }
};
inline struct TestFS {
    std::filesystem::path root;
    auto resolve(const char* path) const { return root/std::filesystem::path(path).relative_path(); }
    File open(const String& path) { return File(resolve(path.c_str())); }
    File open(const char* path) { return File(resolve(path)); }
    bool exists(const char* path) { return std::filesystem::exists(resolve(path)); }
    bool mkdir(const char* path) { std::error_code error; return std::filesystem::create_directory(resolve(path),error); }
} SD_MMC;
