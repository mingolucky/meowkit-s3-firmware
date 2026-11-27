#pragma once
#include <cstdint>
#include <vector>
constexpr uint8_t CARD_NONE=0,CARD_MMC=1,CARD_SD=2,CARD_SDHC=3;
namespace sd_fake {
inline bool inserted=false,mounted=false,rootAvailable=true;
inline unsigned ends=0;
inline std::vector<unsigned> attempts;
inline unsigned maximumFrequency=10000;
}
class File {
    bool _open;
public:
    explicit File(bool open=false) : _open(open) {}
    explicit operator bool() const { return _open; }
    bool isDirectory() const { return _open; }
    void close() { _open=false; }
};
inline struct SdDriver {
    bool setPins(int,int,int) { return !sd_fake::mounted; }
    bool begin(const char*,bool mode1bit,bool format,int khz) {
        if (format || !mode1bit) return false;
        if (sd_fake::mounted) return true;
        sd_fake::attempts.push_back(khz);
        if (!sd_fake::inserted || unsigned(khz)>sd_fake::maximumFrequency) return false;
        sd_fake::mounted=true; return true;
    }
    void end() { ++sd_fake::ends; sd_fake::mounted=false; }
    uint8_t cardType() const { return sd_fake::mounted?CARD_SDHC:CARD_NONE; }
    File open(const char*) { return File(sd_fake::mounted && sd_fake::inserted && sd_fake::rootAvailable); }
    uint64_t cardSize() const { return 8ull*1024*1024*1024; }
    uint64_t totalBytes() const { return cardSize(); }
    uint64_t usedBytes() const { return 1024; }
} SD_MMC;
