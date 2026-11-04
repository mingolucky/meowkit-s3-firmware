#include <mooncake.h>
#include <cstdio>
#include <cstdlib>
#include <memory>

static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

// Only the physical controls and display are mocked. Mooncake's lifecycle
// state machine and the launcher's two production methods are compiled below.
struct Screen {};
static Screen menu, appScreen;
static Screen* ui_apps_menu = &menu;
static Screen* activeScreen = &appScreen;
static Screen* lv_scr_act() { return activeScreen; }
static void lv_disp_load_scr(Screen* screen) { activeScreen = screen; }
static void lv_obj_invalidate(Screen*) {}
static void lv_timer_handler() {}
struct Device {
    struct { void update() {} } led;
    struct {
        struct { bool held = false; bool isLongPress() { return held; } } B;
        void update() {} void tick() {}
    } button;
};
static void power_reset_sleep_timer() {}
static void _update_system_key_sound(Device*) {}

class Launcher {
public:
    Device device;
    Device* _device = &device;
    mooncake::Mooncake _mooncake;
    bool _app_running = false;
    int _running_app_id = -1;
    unsigned returns = 0;
    void updateRunningApp();
    void closeRunningApp();
    void returnToUI() {
        require(!_app_running && _running_app_id == -1,
                "launcher state was not reset before returning to the menu");
        require(activeScreen == ui_apps_menu, "menu was not restored");
        ++returns;
    }
};

#include "launcher_exit_methods.inc"

struct App : mooncake::AppAbility {
    bool requestExit = false;
    unsigned opened = 0, ran = 0, closed = 0;
    void onOpen() override { ++opened; }
    void onRunning() override { ++ran; if (requestExit) close(); }
    void onClose() override {
        require(lv_scr_act() == ui_apps_menu,
                "app cleanup ran before switching away from its active screen");
        ++closed;
    }
};

struct Fixture {
    Launcher launcher;
    App* app;
    int id;
    Fixture() {
        auto instance = std::make_unique<App>(); app = instance.get();
        id = launcher._mooncake.installApp(std::move(instance));
        start();
    }
    void start() {
        activeScreen = &appScreen;
        launcher.device.button.B.held = false;
        app->requestExit = false;
        require(launcher._mooncake.openApp(id), "could not open app");
        launcher._app_running = true; launcher._running_app_id = id;
        launcher.updateRunningApp();
        require(app->currentState() == mooncake::AppAbility::StateRunning, "app did not open");
    }
    void exited(unsigned expected = 1) {
        require(app->currentState() == mooncake::AppAbility::StateSleeping,
                "app did not finish cleanup");
        require(app->closed == expected && launcher.returns == expected,
                "app cleanup or menu return did not run exactly once");
        require(!launcher._app_running && launcher._running_app_id == -1,
                "launcher stayed in APP mode");
    }
};

int main() {
    { Fixture f; f.app->requestExit = true; f.launcher.updateRunningApp(); f.exited(); }
    std::puts("PASS: short B / app-requested exit returns to menu in the same tick");
    { Fixture f; f.app->close(); f.launcher.updateRunningApp(); f.exited();
      require(f.app->ran == 0, "pending close ran the app again"); }
    std::puts("PASS: pending exit completes without an extra running callback");
    { Fixture f; f.launcher.device.button.B.held = true;
      f.launcher.updateRunningApp(); f.exited(); }
    std::puts("PASS: long B uses the same cleanup path");
    { Fixture f; f.app->close(); f.launcher.device.button.B.held = true;
      f.launcher.updateRunningApp(); f.exited(); }
    std::puts("PASS: simultaneous app exit and long B clean up once");
    { Fixture f; f.launcher.updateRunningApp();
      require(f.launcher._app_running && f.app->ran == 1 && !f.app->closed,
              "ordinary navigation unexpectedly closed the app"); }
    std::puts("PASS: navigation without an app close request stays inside the app");
    { Fixture f;
      for (unsigned i = 1; i <= 1000; ++i) {
          if (i != 1) f.start();
          if (i % 2) f.app->requestExit = true;
          else f.launcher.device.button.B.held = true;
          f.launcher.updateRunningApp(); f.exited(i);
      }
      require(f.app->opened == 1000, "reopen count differs"); }
    std::puts("PASS: 1000 repeated opens and short/long B exits");
}
