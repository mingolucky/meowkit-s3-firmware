#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
namespace fonts { inline int efontCN_16; }
class LGFX_Class {
public:
    std::vector<std::string> text;
    void fillScreen(uint32_t) { text.clear(); }
    template<class... A> void fillRect(A...) {}
    template<class... A> void drawRect(A...) {}
    template<class... A> void fillRoundRect(A...) {}
    template<class... A> void drawFastHLine(A...) {}
    template<class... A> void drawFastVLine(A...) {}
    template<class... A> void setTextWrap(A...) {}
    template<class... A> void setFont(A...) {}
    template<class... A> void setTextColor(A...) {}
    template<class... A> void setCursor(A...) {}
    void print(const char* s) { text.emplace_back(s); }
    template<class... A> void printf(const char* format,A... args) {
        char s[256]; std::snprintf(s,sizeof(s),format,args...); text.emplace_back(s);
    }
};
namespace lgfx {
class LGFX_Sprite : public LGFX_Class {
public:
    explicit LGFX_Sprite(LGFX_Class*) {}
    void setPsram(bool) {}
    void setColorDepth(int) {}
    void* createSprite(int,int) { return nullptr; }
    void deleteSprite() {}
    void pushSprite(LGFX_Class*,int,int) {}
};
}
