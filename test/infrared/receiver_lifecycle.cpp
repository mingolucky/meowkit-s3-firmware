#include <IRrecv.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
hw_timer_t hardwareTimer;
void (*gpioHandler)() = nullptr;
bool gpioEnabled = false;
unsigned starts = 0, stops = 0;

void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
void valid(hw_timer_t* timer) {
    require(timer != nullptr, "timer API received a null handle");
    require(timer == &hardwareTimer && timer->active,
            "timer API received an already-ended handle");
}
void receiveSignal() {
    require(gpioHandler && gpioEnabled, "receiver GPIO is not active");
    for (int i = 0; i < 10; ++i) gpioHandler();
    require(hardwareTimer.callback != nullptr, "timeout ISR is not installed");
    hardwareTimer.callback();
}
void balanced() {
    require(starts == stops, "timer allocation/release counts differ");
    require(!hardwareTimer.active && !gpioHandler && !gpioEnabled,
            "hardware remained active after receiver shutdown");
}
} // namespace

HostESP ESP;
void HostESP::restart() { require(false, "unexpected ESP.restart()"); }
// Required by unused protocol-matching methods in IRrecv.cpp. These tests
// capture RAW/hash data and must never enter the protocol matching helpers.
uint64_t reverseBits(uint64_t, uint16_t) {
    require(false, "unexpected protocol bit reversal"); return 0;
}
void pinMode(uint8_t, int) {}
uint32_t micros() { static uint32_t now = 0; return now += 600; }
void attachInterrupt(uint8_t, void (*callback)(), int) {
    valid(&hardwareTimer);
    gpioHandler = callback; gpioEnabled = true;
}
void detachInterrupt(uint8_t) { gpioHandler = nullptr; gpioEnabled = false; }
void gpio_intr_enable(gpio_num_t) { gpioEnabled = true; }
void gpio_intr_disable(gpio_num_t) { gpioEnabled = false; }
hw_timer_t* timerBegin(uint8_t, uint16_t, bool) {
    require(!hardwareTimer.active, "receiver restarted without releasing its timer");
    hardwareTimer.active = true; ++starts;
    return &hardwareTimer;
}
void timerAlarmWrite(hw_timer_t* timer, uint64_t, bool) { valid(timer); }
void timerAttachInterrupt(hw_timer_t* timer, void (*callback)(), bool) {
    valid(timer); timer->callback = callback;
}
void timerAlarmDisable(hw_timer_t* timer) { valid(timer); }
void timerAlarmEnable(hw_timer_t* timer) { valid(timer); }
void timerWrite(hw_timer_t* timer, uint64_t) { valid(timer); }
void timerDetachInterrupt(hw_timer_t* timer) { valid(timer); timer->callback = nullptr; }
void timerEnd(hw_timer_t* timer) {
    valid(timer);
    require(!gpioHandler, "timer released before detaching the receiver GPIO ISR");
    require(!timer->callback, "timer released before detaching the timeout ISR");
    timer->active = false; ++stops;
}

int main(int argc, char** argv) {
    require(argc == 2, "provide a lifecycle scenario");
    const char* scenario = argv[1];
    if (!std::strcmp(scenario, "exit_without_learning")) {
        // B on the app main screen, including a transmit-only session.
        IRrecv receiver(5, 4097, 150, true);
    } else if (!std::strcmp(scenario, "stop_without_start")) {
        IRrecv receiver(5);
        receiver.disableIRIn(); receiver.disableIRIn();
    } else if (!std::strcmp(scenario, "back_from_learning")) {
        IRrecv receiver(5, 4097, 150, true);
        receiver.enableIRIn();
        receiveSignal();
        decode_results results{};
        require(receiver.decode(&results), "captured RAW signal was not available");
        require(results.rawlen == 10, "unexpected capture length");
        receiver.disableIRIn(); receiver.disableIRIn();
    } else if (!std::strcmp(scenario, "exit_while_receiving")) {
        IRrecv receiver(5);
        receiver.enableIRIn(); gpioHandler();
    } else if (!std::strcmp(scenario, "retry_learning")) {
        IRrecv receiver(5, 4097, 150, true);
        for (int i = 0; i < 1000; ++i) {
            receiver.enableIRIn(); receiveSignal(); receiver.disableIRIn();
        }
    } else if (!std::strcmp(scenario, "restart_active_receiver")) {
        IRrecv receiver(5);
        receiver.enableIRIn(); receiver.enableIRIn();
        receiveSignal();
    } else if (!std::strcmp(scenario, "resume_disabled_receiver")) {
        IRrecv receiver(5);
        receiver.resume();
        require(!gpioEnabled, "resume reactivated a receiver without a timer");
        receiver.enableIRIn(); receiver.disableIRIn(); receiver.resume();
        require(!gpioEnabled, "resume reactivated a stopped receiver");
    } else if (!std::strcmp(scenario, "reopen_app")) {
        for (int i = 0; i < 1000; ++i) {
            IRrecv receiver(5, 4097, 150, true);
            if (i % 2) { receiver.enableIRIn(); receiveSignal(); receiver.disableIRIn(); }
        }
    } else {
        require(false, "unknown lifecycle scenario");
    }
    balanced();
    std::printf("PASS: %s (%u timers started, %u stopped)\n", scenario, starts, stops);
}
