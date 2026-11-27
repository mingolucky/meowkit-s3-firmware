#pragma once
#include <string>
namespace mooncake {
class AppAbility {
    struct Info { std::string name; } info;
public:
    virtual ~AppAbility()=default;
    virtual void onOpen() {}
    virtual void onRunning() {}
    virtual void onClose() {}
    Info& setAppInfo() { return info; }
};
}
