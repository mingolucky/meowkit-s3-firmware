/**
 * @file app_ability.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "ability.h"

using namespace mooncake;

void AppAbility::baseCreate()
{
    onCreate();
}

void AppAbility::baseUpdate()
{
    /* ----------------------------------- 状态机 ---------------------------------- */
    switch (_current_state) {
        case StateGoOpen: {
            onOpen();
            _current_state = StateRunning;
            break;
        }
        case StateRunning: {
            onRunning();
            break;
        }
        case StateGoClose: {
            onClose();
            _current_state = StateSleeping;
            break;
        }
        case StateSleeping: {
            onSleeping();
            break;
        }
        default:
            break;
    }
}

void AppAbility::baseDestroy()
{
    onDestroy();
}

void AppAbility::open()
{
    _current_state = StateGoOpen;
}

void AppAbility::close()
{
    _current_state = StateGoClose;
}

const AppAbility::AppInfo_t& AppAbility::getAppInfo()
{
    return _app_info;
}

AppAbility::AppInfo_t& AppAbility::setAppInfo()
{
    return _app_info;
}
