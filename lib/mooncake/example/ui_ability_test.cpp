/**
 * @file ui_ability_test.cpp
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

class MyAppAbility : public UIAbility {
public:
    MyAppAbility()
    {
        printf("[ui] on construct\n");
    }
    ~MyAppAbility()
    {
        printf("[ui] on deconstruct\n");
    }
    void onCreate() override
    {
        printf("[ui] on create\n");
    }
    void onShow() override
    {
        printf("[ui] on show\n");
    }
    void onForeground() override
    {
        printf("[ui] on foreground\n");
    }
    void onHide() override
    {
        printf("[ui] on hide\n");
    }
    void onBackground() override
    {
        printf("[ui] on backgournd\n");
    }
    void onDestroy() override
    {
        printf("[ui] on ondestory\n");
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

    while (1) {
        // 刷新 Ability
        am.updateAbilities();

        // 如果按了回车键
        if (_enter_was_pressed.load()) {
            _enter_was_pressed.store(false);

            // 获取当前状态
            auto ability_current_state = am.getUIAbilityCurrentState(ability_id);

            // 前后台反转
            if (ability_current_state == UIAbility::StateForeground) {
                am.hideUIAbility(ability_id);
            } else if (ability_current_state == UIAbility::StateBackground) {
                am.showUIAbility(ability_id);
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
