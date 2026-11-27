#pragma once
#include <SD_MMC.h>
#include "infrared_builtins.h"

namespace meow_ir {
// The indexer and decoder use identical offsets for SD and flash dictionaries.
class LibraryFile {
    File _file;
    std::shared_ptr<const BuiltinBuffer> _asset;
    size_t _position=0;
public:
    bool open(const String& path) {
        close();
        if (path.startsWith("builtin:")) _asset=openBuiltin(path.c_str()+8);
        else _file=SD_MMC.open(path);
        return bool(*this);
    }
    explicit operator bool() const { return _asset || bool(_file); }
    bool isDirectory() { return !_asset && _file.isDirectory(); }
    size_t size() const { return _asset?_asset->length:_file.size(); }
    size_t position() const { return _asset?_position:_file.position(); }
    int available() { return _asset?int(_asset->length-_position):_file.available(); }
    bool seek(size_t position) {
        if (!_asset) return _file.seek(position);
        if (position>_asset->length) return false;
        _position=position; return true;
    }
    String readStringUntil(char separator) {
        if (!_asset) return _file.readStringUntil(separator);
        const size_t first=_position;
        while (_position<_asset->length && _asset->data.get()[_position]!=uint8_t(separator)) ++_position;
        String line(reinterpret_cast<const char*>(_asset->data.get()+first),unsigned(_position-first));
        if (_position<_asset->length) ++_position;
        return line;
    }
    void close() { _file.close(); _asset.reset(); _position=0; }
};
}
