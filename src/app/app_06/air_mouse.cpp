/** Air Mouse: bounded HID session, non-blocking calibration and motion input. */
#include "air_mouse.h"
#include <Preferences.h>
#include <cmath>
#include <algorithm>

namespace {
// RGB565 palette: near-black forest, lime accents and muted olive surfaces.
constexpr uint16_t BG=0x10C2, GREEN=0xC7A7, DIM=0x6C26, WHITE=0xFFFF;
constexpr uint16_t MESH=0x84E7, PANEL=0x3244, PANEL_ACTIVE=0x42E5, SCANLINE=0x3A65;
constexpr int SX=174, SY=44, SW=142, SH=146;
constexpr float GAINS[]={30.f,50.f,75.f};
const char* SPEEDS[]={"Low","Normal","High"};
void text(lgfx::LGFXBase& lcd,int x,int y,const char* str,uint16_t color=GREEN) {
    lcd.setFont(&fonts::efontCN_16); lcd.setTextDatum(textdatum_t::top_left);
    lcd.setTextColor(color,BG); lcd.setCursor(x,y); lcd.print(str);
}
void globe(lgfx::LGFXBase& lcd,int cy) {
    lcd.drawCircle(88,cy,54,GREEN); lcd.drawCircle(88,cy,53,GREEN);
    lcd.drawEllipse(88,cy,54,19,MESH); lcd.drawEllipse(88,cy,54,42,MESH);
    lcd.drawEllipse(88,cy,18,54,MESH); lcd.drawEllipse(88,cy,38,54,MESH);
    lcd.drawFastHLine(70,cy,36,GREEN); lcd.drawFastVLine(88,cy-18,36,GREEN);
}
void scrollPanel(lgfx::LGFXBase& lcd,bool active) {
    lcd.drawRoundRect(SX,SY,SW,SH,4,GREEN);
    text(lcd,SX+10,SY+4,"Touch Scroll");
    lcd.fillRect(SX+5,SY+22,SW-10,SH-28,active?PANEL_ACTIVE:PANEL);
    for(int y=SY+24;y<SY+SH-6;y+=4)
        lcd.drawFastHLine(SX+8,y,SW-16,active?PANEL:SCANLINE);
}
}
namespace MOONCAKE::APPS {
App06::App06(DEVICES* device):_device(device),_globe(&device->Lcd) {
    setAppInfo().name="Air Mouse";
}
void App06::onOpen() {
    _motion.clear(); _cal.reset(); _calibrated=_calibrating=false;
    _touch_down=_scroll_active=_link_ready=false;
    _a_armed=_b_armed=false; _a_down=_b_down=false;
    _wheel=0; _last_direction=0; _touch_y=-1; _ax=_ay=0;
    _last_status=nullptr; _ui_dirty=true; _ui_scroll=false;
    _sample_us=_poll_us=micros(); _last_gyro=millis(); _last_ui=0; _ignore_until=millis();
    _still_since=0; _bubble_x=_bubble_y=-1000;
    _speed=1; _bias[0]=_bias[1]=_bias[2]=0;
    Preferences prefs;
    if(prefs.begin("airmouse",true)) {
        _speed=std::min<uint8_t>(prefs.getUChar("speed",1),2);
        if(prefs.getUInt("calver",0)==1) {
            float saved[3]={};
            if(prefs.getBytes("bias",saved,sizeof(saved))==sizeof(saved)) {
                bool valid=true;
                for(float v:saved) valid &= std::isfinite(v) && std::fabs(v)<10.f;
                if(valid) {
                    std::copy(saved,saved+3,_bias);
                    _calibrated=true;
                }
            }
        }
        prefs.end();
    }
    auto& b=_device->button;
    b.update(); b.A.hasChanged(); b.B.hasChanged();
    b.Left.hasChanged(); b.Right.hasChanged();
    _previous_range=_device->imu.getGyroRange();
    _imu_ready=_device->imu.isEnabled() && _previous_range!=255 && _device->imu.setGyroRange(2);
    _globe.setColorDepth(16); _globe.setPsram(true); _globe.createSprite(170,142);
    _drawStaticUI();
    air_mouse_hid::begin();
    if(_imu_ready) _calibrate(millis());
    _updateUI();
}
void App06::_calibrate(uint32_t now) {
    if(!_imu_ready) return;
    air_mouse_hid::buttons(0);
    _a_armed=_b_armed=false; _a_down=_b_down=false;
    // Keep the last valid bias until a new attempt succeeds. Use the same
    // clock sample as the timeout check when calibration starts mid-tick.
    _cal.reset(); _calibrating=true;
    _cal_start=now; _still_since=0; _motion.clear(); _wheel=0; _ui_dirty=true;
}
void App06::_setSpeed(uint8_t speed) {
    if(_speed==speed) return;
    _speed=speed; _motion.clear(); _ui_dirty=true;
    Preferences prefs;
    if(prefs.begin("airmouse",false)) { prefs.putUChar("speed",_speed); prefs.end(); }
}
void App06::_inputs(uint32_t now) {
    auto& b=_device->button;
    b.update(); b.tick();
    const bool a=b.A.state()==Button_Class::PRESSED;
    const bool right=b.B.state()==Button_Class::PRESSED;
    const bool ready=air_mouse_hid::ready();
    if(ready!=_link_ready) {
        _link_ready=ready; _motion.clear(); _wheel=0;
        _a_armed=_b_armed=false; _a_down=_b_down=false;
        air_mouse_hid::buttons(0); _ui_dirty=true;
    }
    if(!a) _a_armed=true;
    if(a && !_a_down && _a_armed && ready) {
        air_mouse_hid::buttons(1); _ignore_until=now+60; _motion.clear();
    }
    if(!a && _a_down) air_mouse_hid::buttons(0);
    if(right && !_b_down) { _b_since=now; _motion.clear(); }
    if(!right && _b_down && _b_armed && now-_b_since<1000 && ready) {
        air_mouse_hid::clickRight(); _ignore_until=now+60; _motion.clear();
    }
    if(!right) _b_armed=true;
    _a_down=a; _b_down=right;
    if(b.Left.pressed()) _setSpeed(_speed>0?_speed-1:0);
    if(b.Right.pressed()) _setSpeed(_speed<2?_speed+1:2);
    const bool canScroll=air_mouse_hid::scrollReady();
    if(!canScroll) _wheel=0;
    const int direction=canScroll ? (b.Up.state()==Button_Class::PRESSED?1:0)-(b.Down.state()==Button_Class::PRESSED?1:0) : 0;
    if(direction && (direction!=_last_direction || now-_wheel_at>=160)) {
        _wheel=std::max(-12,std::min(12,_wheel+direction)); _wheel_at=now;
    }
    _last_direction=direction;
    if(direction) _motion.clear();
    _handleTouch(now);
}
void App06::_handleTouch(uint32_t now) {
    int x=-1,y=-1;
    const bool down=_device->ctp.readPoint(x,y) && x>=0 && x<=320 && y>=0 && y<240;
    if(down) {
        const bool canScroll=air_mouse_hid::scrollReady();
        if(!canScroll) { _scroll_active=false; _touch_y=-1; }
        if(!_touch_down) {
            if(y>=194 && y<214) {
                if(x>=160) _calibrate(now); else _setSpeed((_speed+1)%3);
                _ignore_until=now+100; _motion.clear();
            } else if(canScroll && x>=SX && x<SX+SW && y>=SY && y<SY+SH) {
                _scroll_active=true; _touch_y=y; _motion.clear();
            }
        } else if(_scroll_active && _touch_y>=0) {
            const int steps=(y-_touch_y)/10;
            if(steps) {
                _wheel=std::max(-12,std::min(12,_wheel-steps));
                _touch_y+=steps*10;
            }
        }
    } else { _scroll_active=false; _touch_y=-1; }
    _touch_down=down;
}
void App06::_sample(uint32_t now) {
    const uint32_t us=micros();
    if(us-_poll_us<10000 || !_imu_ready) return;
    _poll_us=us;
    auto mask=_device->imu.update();
    const auto& d=_device->imu.getImuData();
    if(mask & IMU_Class::sensor_mask_accel) { _ax=d.accel.x; _ay=d.accel.y; }
    if(!(mask & IMU_Class::sensor_mask_gyro)) return;
    const float dt=(us-_sample_us)*1e-6f; _sample_us=us;
    const float acc=std::sqrt(d.accel.x*d.accel.x+d.accel.y*d.accel.y+d.accel.z*d.accel.z);
    const float raw[3]={d.gyro.x,d.gyro.y,d.gyro.z};
    if(!std::isfinite(acc) || !std::isfinite(raw[0]) || !std::isfinite(raw[1]) || !std::isfinite(raw[2])) {
        _motion.clear(); return;
    }
    // A sample gap invalidates calibration continuity and motion integration.
    const uint32_t gap=now-_last_gyro; _last_gyro=now;
    if(gap>250) { _cal.reset(); _motion.clear(); _still_since=0; return; }
    if(gap>50) { _motion.clear(); _still_since=0; }
    float w[3]={raw[0]-_bias[0],raw[1]-_bias[1],raw[2]-_bias[2]};
    if(_calibrating) {
        if(_cal.add(w[0],w[1],w[2],acc)) {
            for(int i=0;i<3;++i) _bias[i]+=_cal.mean[i];
            _calibrating=false; _calibrated=true; _ui_dirty=true;
            Serial.printf("[App06] Calibration OK: %.3f %.3f %.3f dps\n",_bias[0],_bias[1],_bias[2]);
            Preferences prefs;
            if(prefs.begin("airmouse",false)) {
                prefs.putBytes("bias",_bias,sizeof(_bias)); prefs.putUInt("calver",1); prefs.end();
            }
            _motion.clear(); _ignore_until=now+100;
            return;
        }
    }
    if(!_calibrated) return;
    // Continue pointing with the last good bias while recalibrating.
    const bool still=std::fabs(acc-1)<.03f && w[0]*w[0]+w[1]*w[1]+w[2]*w[2]<.0064f;
    if(!_calibrating && still && !_a_down && !_b_down && !_touch_down) {
        if(!_still_since) _still_since=now;
        if(now-_still_since>2000) for(int i=0;i<3;++i) _bias[i]+=w[i]*std::min(dt/30.f,.001f);
    } else _still_since=0;
    if(!air_mouse_hid::ready() || _scroll_active || _last_direction || _b_down ||
       static_cast<int32_t>(now-_ignore_until)<0) { _motion.clear(); return; }
    _motion.add(w[2],w[1],dt,GAINS[_speed]);
}
void App06::onRunning() {
    const uint32_t now=millis();
    air_mouse_hid::tick();
    _inputs(now);
    if(_device->button.B.isLongPress()) { close(); return; }
    _sample(now);
    if(_calibrating && now-_cal_start>=8000) {
        Serial.printf("[App06] Calibration timeout: samples=%u, mean=%.3f %.3f %.3f dps\n",
            _cal.count,_cal.mean[0],_cal.mean[1],_cal.mean[2]);
        _calibrating=false; _ui_dirty=true;
    }
    if(now-_last_gyro>250) _motion.clear();
    // Buttons and scrolling do not depend on the motion sensor or calibration.
    if(air_mouse_hid::ready()) {
        if(_wheel) {
            const int amount=std::max(-4,std::min(4,_wheel));
            if(air_mouse_hid::move(0,0,amount)) _wheel-=amount;
        } else if(_calibrated) {
            auto r=_motion.pending();
            if((r.x || r.y) && air_mouse_hid::move(r.x,r.y)) _motion.sent(r);
        }
    }
    if(now-_last_ui>=100) { _last_ui=now; _updateUI(); }
    delay(1); // Keep the idle/watchdog task serviced while app mode is active.
}
void App06::onClose() {
    air_mouse_hid::end();
    if(_previous_range!=255 && !_device->imu.setGyroRange(_previous_range))
        Serial.println("[App06] Could not restore gyro range");
    _globe.deleteSprite(); _motion.clear(); _wheel=0;
    _device->Lcd.fillScreen(TFT_BLACK);
}
void App06::_drawStaticUI() {
    auto& lcd=_device->Lcd; lcd.fillScreen(BG);
    text(lcd,8,4,"["); text(lcd,24,4,"AIR MOUSE",WHITE); text(lcd,104,4,"]");
    lcd.drawFastHLine(0,24,320,GREEN); lcd.drawFastHLine(0,214,320,GREEN);
    scrollPanel(lcd,false);
    if(!_globe.getBuffer()) globe(lcd,109);
    text(lcd,8,218,"A: Drag   B: Right / Hold: Exit");
    text(lcd,12,174,"Stick: scroll",DIM);
    _ui_dirty=true;
}
void App06::_updateUI() {
    auto& lcd=_device->Lcd;
    const char* status=air_mouse_hid::failed()?"BLE ERROR":air_mouse_hid::ready()?"CONNECTED":
        air_mouse_hid::connected()?"PAIRING":"WAITING";
    if(status!=_last_status) {
        lcd.fillRect(184,2,136,21,BG); text(lcd,188,4,status); _last_status=status;
    }
    if(_ui_dirty) {
        lcd.fillRect(0,194,320,19,BG);
        char label[24]; snprintf(label,sizeof(label),"Speed: %s",SPEEDS[_speed]);
        text(lcd,8,194,label);
        text(lcd,188,194,!_imu_ready?"IMU ERROR":_calibrating?"KEEP STILL":
            !_calibrated?"TRY AGAIN":"Calibrate"); _ui_dirty=false;
    }
    if(_ui_scroll!=_scroll_active) { scrollPanel(lcd,_scroll_active); _ui_scroll=_scroll_active; }
    float x=_ay*42,y=-_ax*42;
    const float length=std::sqrt(x*x+y*y);
    if(length>46) { x*=46/length; y*=46/length; }
    int bx=88+static_cast<int>(x), by=77+static_cast<int>(y);
    if(_globe.getBuffer() && (bx!=_bubble_x || by!=_bubble_y)) {
        _globe.fillSprite(BG); globe(_globe,77);
        _globe.fillCircle(bx,by,6,GREEN); _globe.fillCircle(bx,by,3,WHITE);
        _globe.pushSprite(0,32); _bubble_x=bx; _bubble_y=by;
    }
}
}
