/**
 * @file basic_demo.cpp
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

// 派生一个 App
class MyApp : public AppAbility {
public:
    MyApp()
    {
        // 配置 App 信息
        setAppInfo().name = "凤爪";

        printf("[%s] on construct\n", getAppInfo().name.c_str());
    }

    // 重载生命周期回调函数
    void onOpen() override
    {
        printf("[%s] on open\n", getAppInfo().name.c_str());
    }
    void onRunning() override
    {
        printf("[%s] on running\n", getAppInfo().name.c_str());
    }

    // 省略
    // void onCreate() override {}
    // void onSleeping() override {}
    // void onClose() override {}
    // void onDestroy() override {}
};

int main()
{
    // 创建 Mooncake
    Mooncake mc;

    // 安装 App
    printf(">> install app\n");
    auto my_app_id = mc.installApp(std::make_unique<MyApp>());
    // >> install app
    // [凤爪] on construct

    // 打开 App
    mc.openApp(my_app_id);

    // 更新 Mooncake
    printf(">> update mooncake\n");
    for (int i = 0; i < 3; i++) {
        mc.update();
    }
    // >> update mooncake
    // [凤爪] on open
    // [凤爪] on running
    // [凤爪] on running

    return 0;
}
