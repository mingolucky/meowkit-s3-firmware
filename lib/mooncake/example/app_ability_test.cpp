/**
 * @file app_ability_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <ability/ability.h>
#include <ability_manager/ability_manager.h>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace mooncake;

std::atomic<bool> _enter_was_pressed{false};

class MyAppAbility : public AppAbility {
public:
    MyAppAbility()
    {
        printf("[app] on construct\n");

        // 配置 App 信息
        setAppInfo().name = "帅的不谈";
    }
    ~MyAppAbility()
    {
        printf("[app] on deconstruct\n");
    }
    void onCreate() override
    {
        printf("[app] on create\n");
    }
    void onOpen() override
    {
        printf("[app] on open\n");
    }
    void onRunning() override
    {
        printf("[app] on running\n");
    }
    void onSleeping() override
    {
        printf("[app] on sleeping\n");
    }
    void onClose() override
    {
        printf("[app] on close\n");
    }
    void onDestroy() override
    {
        printf("[app] on ondestory\n");
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

    AbilityManager am;

    // 创建 Ability
    printf(">> create ability\n");
    auto ability_id = am.createAbility(std::make_unique<MyAppAbility>());

    // 获取 App 信息
    printf(">> app name is: %s\n", am.getAppAbilityAppInfo(ability_id).name.c_str());

    while (1) {
        // 刷新 Ability
        am.updateAbilities();

        // 如果按了回车键
        if (_enter_was_pressed.load()) {
            _enter_was_pressed.store(false);

            // 获取当前状态
            auto ability_current_state = am.getAppAbilityCurrentState(ability_id);

            // 打开关闭反转
            if (ability_current_state == AppAbility::StateRunning) {
                am.closeAppAbility(ability_id);
            } else if (ability_current_state == AppAbility::StateSleeping) {
                am.openAppAbility(ability_id);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    // 销毁 Ability
    printf(">> destroy ability\n");
    am.destroyAbility(ability_id);
    am.updateAbilities();

    return 0;
}
