/**
 * @file ability_manager.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-26
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "ability_manager.h"
#include "ability/ability.h"
#include <iterator>

using namespace mooncake;

int AbilityManager::createAbility(std::unique_ptr<AbilityBase> ability)
{
    if (!ability) {
        return -1;
    }

    // 分配 Ability ID
    AbilityInfo_t new_ability_info;
    new_ability_info.id = get_next_ability_id();
    new_ability_info.state = StateGoCreate;
    new_ability_info.ability = std::move(ability);

    // 转移所有权，暂存到 new ability list
    _new_ability_list.push_back(std::move(new_ability_info));

    return new_ability_info.id;
}

bool AbilityManager::destroyAbility(int abilityID)
{
    // 遍历查找对应 ID 的 Ability
    for (auto& ability_info : _ability_list) {
        if (ability_info.id == abilityID) {
            // 更新状态
            ability_info.state = StateGoDestroy;
            return true;
        }
    }
    for (auto& ability_info : _new_ability_list) {
        if (ability_info.id == abilityID) {
            // 更新状态
            ability_info.state = StateGoDestroy;
            return true;
        }
    }
    return false;
}

void AbilityManager::updateAbilities()
{
    // 如果有新添加的 ability
    if (!_new_ability_list.empty()) {
        // 合并到 ability list
        std::move(_new_ability_list.begin(), _new_ability_list.end(), std::back_inserter(_ability_list));
        _new_ability_list.clear();
    }

    // 遍历所有 Abilities
    for (auto ability_iter = _ability_list.begin(); ability_iter != _ability_list.end();) {
        update_ability(ability_iter);
    }
}

void AbilityManager::updateAbility(int abilityID)
{
    // 如果有新添加的 ability
    if (!_new_ability_list.empty()) {
        // 合并到 ability list
        std::move(_new_ability_list.begin(), _new_ability_list.end(), std::back_inserter(_ability_list));
        _new_ability_list.clear();
    }

    // 遍历所有 Abilities
    for (auto ability_iter = _ability_list.begin(); ability_iter != _ability_list.end(); ability_iter++) {
        if (ability_iter->id == abilityID) {
            update_ability(ability_iter);
            break;
        }
    }
}

std::size_t AbilityManager::getAbilityNum()
{
    return _ability_list.size() + _new_ability_list.size();
}

bool AbilityManager::isAbilityExist(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        return true;
    }
    return false;
}

AbilityBase* AbilityManager::getAbilityInstance(int abilityID)
{
    // 遍历查找对应 ID 的 Ability
    for (auto& ability_info : _new_ability_list) {
        if (ability_info.id == abilityID) {
            return ability_info.ability.get();
        }
    }
    for (auto& ability_info : _ability_list) {
        if (ability_info.id == abilityID) {
            return ability_info.ability.get();
        }
    }
    return nullptr;
}

std::vector<AbilityBase*> AbilityManager::getAllAbilityInstance()
{
    std::vector<AbilityBase*> ret;

    for (auto& ability_info : _ability_list) {
        ret.push_back(ability_info.ability.get());
    }
    for (auto& ability_info : _new_ability_list) {
        ret.push_back(ability_info.ability.get());
    }

    return ret;
}

AbilityType_t AbilityManager::getAbilityType(int abilityID)
{
    auto ability_instance = getAbilityInstance(abilityID);
    if (ability_instance) {
        return ability_instance->abilityType();
    }
    return AbilityType_Null;
}

int AbilityManager::get_next_ability_id()
{
    int next_ability_id = -1;

    // 检查可用 ID 列表
    if (!_available_ability_id_list.empty()) {
        next_ability_id = _available_ability_id_list.front();
        _available_ability_id_list.erase(_available_ability_id_list.begin());
        return next_ability_id;
    }

    // 如果没有，继续自加
    next_ability_id = _next_ability_id;
    _next_ability_id++;

    return next_ability_id;
}

void AbilityManager::update_ability(std::vector<mooncake::AbilityManager::AbilityInfo_t>::iterator& abilityIter)
{
    /* ----------------------------------- 状态机 ---------------------------------- */
    // 更新状态，触发原始回调
    switch (abilityIter->state) {
        case StateGoCreate: {
            // 触发创建回调
            abilityIter->ability->baseCreate();
            // 切到正常刷新状态
            abilityIter->state = StateUpdating;
            abilityIter->ability->baseUpdate();
            abilityIter++;
            break;
        }
        case StateUpdating: {
            abilityIter->ability->baseUpdate();
            abilityIter++;
            break;
        }
        case StateGoDestroy: {
            // 触发销毁回调
            abilityIter->ability->baseDestroy();
            // 把 ID 存到可用 ID 列表，防止反复创建销毁导致的 ID 溢出
            _available_ability_id_list.push_back(abilityIter->id);
            // 销毁 Ability
            abilityIter = _ability_list.erase(abilityIter);
            break;
        }
        default: {
            abilityIter++;
            break;
        }
    }
}
