/**
 * @file extension_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <cstdio>
#include <memory>
#include <mooncake.h>

using namespace mooncake;

class MyApp : public AppAbility {
public:
    MyApp()
    {
        printf("[app] on construct\n");
        // 打开 App
        open();
    }
    ~MyApp()
    {
        printf("[app] on deconstruct\n");
    }
    void onRunning() override
    {
        printf("[app] on running\n");
    }
};

class MyWorker : public WorkerAbility {
public:
    MyWorker()
    {
        printf("[worker] on construct\n");
    }
    ~MyWorker()
    {
        printf("[worker] on deconstruct\n");
    }
    void onRunning() override
    {
        printf("[worker] on running\n");
    }
};

class MyUI : public UIAbility {
public:
    MyUI()
    {
        printf("[ui] on construct\n");
    }
    ~MyUI()
    {
        printf("[ui] on deconstruct\n");
    }
    void onForeground() override
    {
        printf("[ui] on foreground\n");
    }
};

int main()
{
    // 创建 Mooncake
    Mooncake mc;

    // 安装 App
    printf(">> install app\n");
    mc.installApp(std::make_unique<MyApp>());
    // >> install app
    // [app] on construct

    // 安装扩展
    printf(">> install extensions\n");
    mc.createExtension(std::make_unique<MyWorker>());
    mc.createExtension(std::make_unique<MyUI>());
    // >> install extensions
    // [worker] on construct
    // [ui] on construct

    // 更新 Mooncake
    printf(">> update mooncake\n");
    for (int i = 0; i < 3; i++) {
        mc.update();
    }
    // >> update mooncake
    // [worker] on running
    // [ui] on foreground
    // [app] on running
    // [worker] on running
    // [ui] on foreground
    // [app] on running
    // [worker] on running
    // [ui] on foreground

    printf(">> test over\n");
    return 0;
    // >> test over
    // [worker] on deconstruct
    // [ui] on deconstruct
    // [app] on deconstruct
}
