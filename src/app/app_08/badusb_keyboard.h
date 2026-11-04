#pragma once
#include <cstdint>
#include <cstring>
// ASCII table adapted from the existing ESP32-BLE-Keyboard transport.
namespace badusb_hid {
inline constexpr uint8_t asciiMap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US

	0x2c,		   //  ' '
	0x1e|0x80,	   // !
	0x34|0x80,	   // "
	0x20|0x80,    // #
	0x21|0x80,    // $
	0x22|0x80,    // %
	0x24|0x80,    // &
	0x34,          // '
	0x26|0x80,    // (
	0x27|0x80,    // )
	0x25|0x80,    // *
	0x2e|0x80,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|0x80,      // :
	0x33,          // ;
	0x36|0x80,      // <
	0x2e,          // =
	0x37|0x80,      // >
	0x38|0x80,      // ?
	0x1f|0x80,      // @
	0x04|0x80,      // A
	0x05|0x80,      // B
	0x06|0x80,      // C
	0x07|0x80,      // D
	0x08|0x80,      // E
	0x09|0x80,      // F
	0x0a|0x80,      // G
	0x0b|0x80,      // H
	0x0c|0x80,      // I
	0x0d|0x80,      // J
	0x0e|0x80,      // K
	0x0f|0x80,      // L
	0x10|0x80,      // M
	0x11|0x80,      // N
	0x12|0x80,      // O
	0x13|0x80,      // P
	0x14|0x80,      // Q
	0x15|0x80,      // R
	0x16|0x80,      // S
	0x17|0x80,      // T
	0x18|0x80,      // U
	0x19|0x80,      // V
	0x1a|0x80,      // W
	0x1b|0x80,      // X
	0x1c|0x80,      // Y
	0x1d|0x80,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|0x80,    // ^
	0x2d|0x80,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|0x80,    // {
	0x31|0x80,    // |
	0x30|0x80,    // }
	0x35|0x80,    // ~
	0				// DEL
};
inline uint16_t decode(uint8_t key, const uint16_t* layout=nullptr) {
    if (key >= 136) return key - 136;
    if (key >= 128) return uint16_t(1u << (key - 128)) << 8;
    if (layout) return layout[key];
    const uint8_t value = asciiMap[key];
    return (value & 0x7f) | ((value & 0x80) ? 0x0200 : 0);
}
struct Keyboard {
    uint8_t bytes[8] = {};
    uint8_t modifierRefs[8] = {};
    bool press(uint16_t key) {
        const uint8_t usage = key & 0xff, mods = key >> 8;
        if (!key || usage >= 0xe0) return false;
        if (usage) {
            bool found=false;
            for (unsigned i=2;i<8;++i) if (bytes[i]==usage) found=true;
            if (!found) for (unsigned i=2;i<8;++i) if (!bytes[i]) { bytes[i]=usage;found=true;break; }
            if (!found) return false;
        }
        for(unsigned bit=0;bit<8;++bit) if(mods & (1u<<bit)) {
            if(modifierRefs[bit]<255) ++modifierRefs[bit];
            bytes[0] |= 1u<<bit;
        }
        return true;
    }
    void release(uint16_t key) {
        const uint8_t usage=key&0xff,mods=key>>8;
        if(usage) for(unsigned i=2;i<8;++i) if(bytes[i]==usage) bytes[i]=0;
        for(unsigned bit=0;bit<8;++bit) if(mods & (1u<<bit)) {
            if(modifierRefs[bit]) --modifierRefs[bit];
            if(!modifierRefs[bit]) bytes[0] &= ~(1u<<bit);
        }
    }
    void clear() { memset(bytes,0,sizeof(bytes));memset(modifierRefs,0,sizeof(modifierRefs)); }
};
} // namespace badusb_hid
