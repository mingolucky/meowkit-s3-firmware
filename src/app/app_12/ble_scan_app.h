#pragma once
#include <mooncake.h>
#include "../../bsp/devices.h"
#include "ble_scanner.h"

namespace MOONCAKE::APPS {
class App12 : public mooncake::AppAbility {
public:
    explicit App12(DEVICES* device);
    void onOpen() override;
    void onRunning() override;
    void onClose() override;
private:
    enum class Page : uint8_t { List,Info,Raw };
    DEVICES* device_;
    ble_scan::Scanner scanner_;
    lgfx::LGFX_Sprite* canvas_=nullptr;
    bool haveCanvas_=false,armed_=false,touched_=false,palOnly_=false,dirty_=true;
    bool haveFocus_=false;
    uint8_t focus_[6]{},focusType_=0;
    std::array<unsigned,ble_scan::Table::Capacity> order_{};
    unsigned count_=0,selected_=0,top_=0;
    std::array<ble_scan::Device,5> frame_{};
    unsigned frameCount_=0,frameTop_=0;
    ble_scan::Device touchDevice_{};
    ble_scan::Device detail_{};
    Page page_=Page::List;
    int touchAction_=-1,touchX_=0,touchY_=0;
    uint32_t lastDraw_=0;
    void rebuild();
    void remember(bool displayed=true);
    void input();
    void activate();
    void back();
    void filter();
    int hit(int x,int y) const;
    void present();
    template<class LCD> void render(LCD& lcd);
};
} // namespace MOONCAKE::APPS
