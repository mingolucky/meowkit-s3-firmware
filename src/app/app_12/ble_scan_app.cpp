#include "ble_scan_app.h"
#include "../app_common/hp_ui.h"
#include <Arduino.h>
#include <algorithm>
#include <cstdlib>
#include <new>

namespace MOONCAKE::APPS {
using namespace ble_scan;
App12::App12(DEVICES* device) : device_(device) { setAppInfo().name="BLE Scanner"; }
void App12::onOpen() {
    page_=Page::List; palOnly_=false; haveFocus_=false;
    count_=selected_=top_=frameCount_=frameTop_=0; armed_=touched_=false; touchAction_=-1;
    canvas_=new(std::nothrow) lgfx::LGFX_Sprite(&device_->Lcd);
    haveCanvas_=false;
    if(canvas_) {
        canvas_->setPsram(true); canvas_->setColorDepth(16);
        haveCanvas_=canvas_->createSprite(320,240)!=nullptr;
    }
    scanner_.begin(); dirty_=true; present();
}
void App12::onClose() {
    scanner_.end();
    if(canvas_) { canvas_->deleteSprite(); delete canvas_; canvas_=nullptr; }
    haveCanvas_=false;
}
void App12::remember(bool displayed) {
    if(selected_>=count_) { haveFocus_=false; return; }
    const auto& d=displayed && selected_>=frameTop_ && selected_<frameTop_+frameCount_
        ? frame_[selected_-frameTop_] : scanner_.table()[order_[selected_]];
    std::memcpy(focus_,d.address,6); focusType_=d.addressType; haveFocus_=true;
}
void App12::rebuild() {
    const auto& table=scanner_.table();
    count_=0;
    for(unsigned i=0;i<table.count();++i)
        if(!palOnly_ || table[i].isPal()) order_[count_++]=i;
    std::sort(order_.begin(),order_.begin()+count_,[&](unsigned a,unsigned b) {
        const auto& x=table[a]; const auto& y=table[b];
        if(x.matchesTarget()!=y.matchesTarget()) return x.matchesTarget();
        if(x.isPal()!=y.isPal()) return x.isPal();
        if(x.rssi!=y.rssi) return x.rssi>y.rssi;
        const int address=std::memcmp(x.address,y.address,6);
        return address ? address<0 : x.addressType<y.addressType;
    });
    selected_=0;
    for(unsigned i=0;haveFocus_ && i<count_;++i)
        if(table[order_[i]].sameAddress(focus_,focusType_)) { selected_=i; break; }
    if(!count_) top_=0;
    else {
        if(selected_<top_) top_=selected_;
        if(selected_>=top_+5) top_=selected_-4;
        top_=std::min(top_,count_>5 ? count_-5 : 0);
    }
    remember(false);
    if(page_!=Page::List) {
        for(unsigned i=0;i<table.count();++i)
            if(table[i].sameAddress(detail_.address,detail_.addressType)) { detail_=table[i]; break; }
    }
}
void App12::filter() {
    palOnly_=!palOnly_; top_=0; rebuild(); dirty_=true;
}
void App12::activate() {
    if(page_==Page::List) {
        if(scanner_.state()==Scanner::State::Error) {
            scanner_.begin(); haveFocus_=false; rebuild();
        } else if(selected_>=frameTop_ && selected_<frameTop_+frameCount_) {
            detail_=frame_[selected_-frameTop_]; page_=Page::Info;
        }
    } else page_=page_==Page::Info ? Page::Raw : Page::Info;
    armed_=false; dirty_=true;
}
void App12::back() {
    if(page_==Page::List) scanner_.setPaused(!scanner_.paused());
    else page_=Page::List;
    armed_=false; dirty_=true;
}
int App12::hit(int x,int y) const {
    if(x<0 || x>=320 || y<0 || y>=240) return -1;
    if(y>=215) return x<160 ? 100 : 101;
    if(page_==Page::List) {
        if(y>=27 && y<49) return 102;
        if(y>=51 && y<211) {
            const unsigned row=unsigned((y-51)/32);
            return row<frameCount_ ? static_cast<int>(row) : -1;
        }
    }
    return -1;
}
void App12::input() {
    auto& b=device_->button; b.update(); b.tick();
    bool a=b.A.pressed(),backPressed=b.B.pressed();
    const bool up=b.Up.pressed(),down=b.Down.pressed();
    const bool left=b.Left.pressed(),right=b.Right.pressed();
    int x=-1,y=-1;
    const bool touch=device_->ctp.readPoint(x,y) && x>=0 && x<320 && y>=0 && y<240;
    if(!armed_) {
        touched_=touch;
        if(!touch && b.A.state()==Button_Class::RELEASED && b.B.state()==Button_Class::RELEASED &&
           b.Up.state()==Button_Class::RELEASED && b.Down.state()==Button_Class::RELEASED &&
           b.Left.state()==Button_Class::RELEASED && b.Right.state()==Button_Class::RELEASED) armed_=true;
        return;
    }
    if(touch && !touched_) {
        touchX_=x; touchY_=y; touchAction_=hit(x,y);
        if(touchAction_>=0 && touchAction_<5) touchDevice_=frame_[static_cast<unsigned>(touchAction_)];
    }
    if(touch && touched_ && (std::abs(x-touchX_)>15 || std::abs(y-touchY_)>15)) touchAction_=-1;
    if(!touch && touched_ && touchAction_>=0) {
        if(touchAction_==100) a=true;
        else if(touchAction_==101) backPressed=true;
        else if(touchAction_==102) filter();
        else {
            detail_=touchDevice_; page_=Page::Info; armed_=false; dirty_=true;
        }
        touchAction_=-1;
    }
    touched_=touch;
    if(backPressed) { back(); return; }
    if(page_==Page::List) {
        if(left || right) filter();
        if(count_ && (up || down)) {
            if(up && selected_) --selected_;
            if(down && selected_+1<count_) ++selected_;
            if(selected_<top_) top_=selected_;
            if(selected_>=top_+5) top_=selected_-4;
            remember(); dirty_=true;
        }
    } else if(left || right) { activate(); return; }
    if(a) activate();
}
void App12::onRunning() {
    scanner_.poll();
    input();
    if(!touched_ && (dirty_ || millis()-lastDraw_>=500)) present();
    delay(10);
}
template<class LCD> void App12::render(LCD& lcd) {
    lcd.fillScreen(hp::COL_BG); lcd.setTextWrap(false,false);
    hp::drawChrome(lcd);
    const auto state=scanner_.state();
    const char* badge=state==Scanner::State::Error ? "ERROR" :
        state==Scanner::State::Scanning ? "SCAN" : scanner_.paused() ? "PAUSED" : "START";
    hp::drawHeader(lcd,"BLE SCANNER",badge);
    lcd.setFont(&fonts::efontCN_16);
    auto line=[&](int y,const char* value,uint16_t color=hp::COL_FG) {
        lcd.setTextColor(color,hp::COL_BG); lcd.setCursor(8,y); lcd.print(value);
    };
    char text[80];
    if(page_==Page::List) {
        std::snprintf(text,sizeof(text),"%s %u  PAL %u  drop %lu  L/R:filter",palOnly_?"PAL":"All",
            scanner_.table().count(),scanner_.table().palCount(),static_cast<unsigned long>(scanner_.dropped()));
        line(29,text,hp::COL_DIM);
        if(state==Scanner::State::Error) {
            line(72,scanner_.message(),hp::COL_WARN);
            std::snprintf(text,sizeof(text),"Error: %d",scanner_.error()); line(100,text);
            line(135,"A: retry   Hold B: exit",hp::COL_DIM);
        } else if(!count_) {
            line(68,palOnly_?"No PAL candidates detected yet":"No BLE devices detected yet");
            std::snprintf(text,sizeof(text),"Target: %s",TargetId); line(99,text,hp::COL_DIM);
            line(133,"Wait 30-60 seconds near the gate.",hp::COL_DIM);
            line(165,"Not detected is not proof of no BLE.",hp::COL_DIM);
        } else {
            for(unsigned row=0;row<5 && top_+row<count_;++row) {
                const unsigned i=top_+row;
                const auto& d=scanner_.table()[order_[i]];
                const int y=51+static_cast<int>(row)*32;
                const bool selected=i==selected_;
                const uint16_t bg=selected?hp::COL_FG:hp::COL_BG;
                lcd.fillRect(3,y,306,32,bg);
                lcd.setTextColor(selected?hp::COL_BG:hp::COL_FG,bg); lcd.setCursor(8,y);
                const char* label=d.palId[0]?d.palId:d.name[0]?d.name:"Unnamed";
                std::snprintf(text,sizeof(text),"%s%.22s %d dBm",d.matchesTarget()?"* ":d.isPal()?"P ":"  ",label,int(d.rssi));
                lcd.print(text);
                char address[18]; addressText(d.address,address);
                std::snprintf(text,sizeof(text),"%s %lus%s",address,
                    static_cast<unsigned long>((millis()-d.lastSeen)/1000),d.isPal()?" PAL":"");
                lcd.setTextColor(selected?hp::COL_BG:hp::COL_DIM,bg); lcd.setCursor(24,y+16); lcd.print(text);
            }
            hp::drawScrollbar(lcd,static_cast<int>(count_),static_cast<int>(top_),5);
        }
        hp::drawFooter(lcd,state==Scanner::State::Error?"[A] Retry":"[A] Details",
            scanner_.paused()?"Resume [B]":"Pause [B]");
    } else {
        char address[18]; addressText(detail_.address,address);
        if(page_==Page::Info) {
            std::snprintf(text,sizeof(text),"Name: %.30s",detail_.name[0]?detail_.name:"(not advertised)"); line(31,text);
            std::snprintf(text,sizeof(text),"%s %s",address,detail_.addressType==0?"public":"random"); line(53,text);
            std::snprintf(text,sizeof(text),"Signal: %d dBm   Seen: %lus ago",int(detail_.rssi),
                static_cast<unsigned long>((millis()-detail_.lastSeen)/1000)); line(75,text);
            std::snprintf(text,sizeof(text),"Reports: %lu",static_cast<unsigned long>(detail_.packets)); line(97,text);
            std::snprintf(text,sizeof(text),"PAL ID: %s",detail_.palId[0]?detail_.palId:"not observed"); line(119,text);
            line(141,detail_.matchesTarget()?"TARGET ID MATCH":detail_.palService?"PAL service UUID advertised":
                detail_.palId[0]?"PAL controller ID advertised":detail_.palName?"PAL name hint only":"No PAL identifier observed",
                detail_.isPal()?hp::COL_ACCENT:hp::COL_DIM);
            std::snprintf(text,sizeof(text),"Adv: %u bytes  Response: %u bytes",detail_.advLength,detail_.responseLength); line(163,text);
            line(187,"Hold B anywhere to exit",hp::COL_DIM);
        } else {
            line(29,"Advertisement + scan response",hp::COL_DIM);
            const unsigned total=detail_.advLength+detail_.responseLength;
            for(unsigned row=0;row<6;++row) {
                const unsigned offset=row*11;
                unsigned used=0;
                for(unsigned i=offset;i<total && i<offset+11;++i) {
                    const uint8_t byte=i<detail_.advLength?detail_.adv[i]:detail_.response[i-detail_.advLength];
                    used+=static_cast<unsigned>(std::snprintf(text+used,sizeof(text)-used,"%02X ",byte));
                }
                if(!used) text[0]=0;
                line(56+static_cast<int>(row)*24,text);
            }
        }
        hp::drawFooter(lcd,page_==Page::Info?"[A] Raw data":"[A] Info","Back [B]");
    }
}
void App12::present() {
    rebuild();
    frameTop_=top_; frameCount_=std::min(5u,count_-top_);
    for(unsigned row=0;row<frameCount_;++row) frame_[row]=scanner_.table()[order_[top_+row]];
    if(haveCanvas_) { render(*canvas_); canvas_->pushSprite(&device_->Lcd,0,0); }
    else render(device_->Lcd);
    lastDraw_=millis(); dirty_=false;
}
} // namespace MOONCAKE::APPS
