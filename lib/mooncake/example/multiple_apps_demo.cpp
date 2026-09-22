/**
 * @file multiple_apps_demo.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "ability/ability.h"
#include <thread>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mooncake.h>

using namespace mooncake;

std::atomic<bool> _enter_was_pressed{false};

/**
 * @brief 派生一个 App
 *
 */
class App_1 : public AppAbility {
public:
    App_1()
    {
        printf("[1] on construct\n");

        // 配置 App 信息
        setAppInfo().name = "肠粉";
    }
    void onOpen() override
    {
        printf("[1] on open\n");
    }
    void onRunning() override
    {
        printf("[1] [%s] on running\n", getAppInfo().name.c_str());
    }
    void onClose() override
    {
        printf("[1] on close\n");
    }
};

/**
 * @brief 派生另一个 App
 *
 */
class App_2 : public AppAbility {
public:
    App_2()
    {
        printf("[2] on construct\n");

        // 配置 App 信息
        setAppInfo().name = "虾饺";
    }
    void onOpen() override
    {
        printf("[2] on open\n");
    }
    void onRunning() override
    {
        printf("[2] [%s] on running\n", getAppInfo().name.c_str());
    }
    void onClose() override
    {
        printf("[2] on close\n");
    }
};

int main()
{
    // 创建一个线程监听回车键
    std::thread enter_listener([] {
        std::string dev_null;
        while (1) {
            std::getline(std::cin, dev_null);
            _enter_was_pressed.store(true);
        }
    });

    // 创建 Mooncake
    Mooncake mc;

    // 安装 App
    auto app_1_id = mc.installApp(std::make_unique<App_1>());
    auto app_2_id = mc.installApp(std::make_unique<App_2>());
    // [1] on construct
    // [2] on construct

    // 获取 App 信息
    auto app_info_list = mc.getAllAppInfo();
    printf(">> app num: %zu\n", app_info_list.size());
    for (auto& info : app_info_list) {
        printf(">> app name: %s\n", info.name.c_str());
    }
    // >> app num: 2
    // >> app name: 肠粉
    // >> app name: 虾饺

    // 轮流操控 App_1、App_2
    bool is_app_1_turn = true;

    while (1) {
        // 更新 Mooncake
        mc.update();

        // 如果按了回车键
        if (_enter_was_pressed.load()) {
            _enter_was_pressed.store(false);

            if (is_app_1_turn) {
                // 反转 App_1 打开关闭状态
                if (mc.getAppCurrentState(app_1_id) == AppAbility::StateSleeping) {
                    mc.openApp(app_1_id);
                } else {
                    mc.closeApp(app_1_id);
                    is_app_1_turn = false;
                }

            } else {
                // 反转 App_2 打开关闭状态
                if (mc.getAppCurrentState(app_2_id) == AppAbility::StateSleeping) {
                    mc.openApp(app_2_id);
                } else {
                    mc.closeApp(app_2_id);
                    is_app_1_turn = true;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return 0;
}