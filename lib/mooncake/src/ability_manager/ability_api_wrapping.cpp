/**
 * @file ability_api_wrapping.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "ability_manager.h"

using namespace mooncake;

/* -------------------------------------------------------------------------- */
/*                                 UI Ability                                 */
/* -------------------------------------------------------------------------- */

bool AbilityManager::showUIAbility(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_UI) {
            static_cast<UIAbility*>(ability_instance)->show();
            return true;
        }
    }
    return false;
}

bool AbilityManager::hideUIAbility(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_UI) {
            static_cast<UIAbility*>(ability_instance)->hide();
            return true;
        }
    }
    return false;
}

UIAbility::State_t AbilityManager::getUIAbilityCurrentState(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_UI) {
            return static_cast<UIAbility*>(ability_instance)->currentState();
        }
    }
    return UIAbility::StateNull;
}

/* -------------------------------------------------------------------------- */
/*                               Worker Ability                               */
/* -------------------------------------------------------------------------- */

bool AbilityManager::pauseWorkerAbility(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_Worker) {
            static_cast<WorkerAbility*>(ability_instance)->pause();
            return true;
        }
    }
    return false;
}

bool AbilityManager::resumeWorkerAbility(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_Worker) {
            static_cast<WorkerAbility*>(ability_instance)->resume();
            return true;
        }
    }
    return false;
}

WorkerAbility::State_t AbilityManager::getWorkerAbilityCurrentState(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_Worker) {
            return static_cast<WorkerAbility*>(ability_instance)->currentState();
        }
    }
    return WorkerAbility::StateNull;
}

/* -------------------------------------------------------------------------- */
/*                                 App Ability                                */
/* -------------------------------------------------------------------------- */

bool AbilityManager::openAppAbility(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_App) {
            static_cast<AppAbility*>(ability_instance)->open();
            return true;
        }
    }
    return false;
}

bool AbilityManager::closeAppAbility(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_App) {
            static_cast<AppAbility*>(ability_instance)->close();
            return true;
        }
    }
    return false;
}

AppAbility::AppInfo_t AbilityManager::getAppAbilityAppInfo(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_App) {
            return static_cast<AppAbility*>(ability_instance)->getAppInfo();
        }
    }
    return AppAbility::AppInfo_t();
}

AppAbility::State_t AbilityManager::getAppAbilityCurrentState(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        // 类型校验
        if (ability_instance->abilityType() == AbilityType_App) {
            return static_cast<AppAbility*>(ability_instance)->currentState();
        }
    }
    return AppAbility::StateNull;
}
