/**
 * @file ability_manager_test.cpp
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
#include <cstdio>
#include <memory>

using namespace mooncake;

/**
 * @brief 简单派生一个 Basic Ability
 *
 */
class BasicAbilityTest : public BasicAbility {
public:
    BasicAbilityTest()
    {
        printf("[basic] on construct\n");
    }
    ~BasicAbilityTest()
    {
        printf("[basic] on deconstruct\n");
    }
    void onCreate() override
    {
        printf("[basic] on create\n");
    }
    void onRunning() override
    {
        printf("[basic] on running\n");
    }
    void onDestroy() override
    {
        printf("[basic] on ondestory\n");
    }
};

int main()
{
    AbilityManager am;

    // 创建 Ability
    printf(">> create ability\n");
    auto basic_ability_id = am.createAbility(std::make_unique<BasicAbilityTest>());
    // >> create ability
    // [basic] on construct

    // 刷新 Ability
    printf(">> update ability for 3 times\n");
    for (int i = 0; i < 3; i++) {
        am.updateAbilities();
    }
    // [basic] on create
    // [basic] on running
    // [basic] on running
    // [basic] on running

    // 销毁 Ability
    printf(">> destroy basic ability\n");
    am.destroyAbility(basic_ability_id);
    am.updateAbilities();
    // >> destroy basic ability
    // [basic] on ondestory
    // [basic] on deconstruct

    // 超出作用域自动释放
    printf(">> test over\n");
    am.createAbility(std::make_unique<BasicAbilityTest>());
    // >> test over
    // [basic] on construct
    // [basic] on deconstruct
    return 0;
}
