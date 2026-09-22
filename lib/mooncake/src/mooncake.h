/**
 * @file mooncake.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#include "ability/ability.h"
#include "ability_manager/ability_manager.h"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#define MOONCAKE_VERSION "2.0.0"

namespace mooncake {

class Mooncake {
public:
    /**
     * @brief 打印关于信息
     *
     */
    void logAboutMsg();

    /**
     * @brief 更新 Mooncake，刷新 App 及 Extension 状态，触发生命周期回调
     *
     */
    void update();

    /* -------------------------------------------------------------------------- */
    /*                             App Ability Manager                            */
    /* -------------------------------------------------------------------------- */
    // App Ability 管理器 API 封装

    /**
     * @brief 安装 App，返回 App ID
     *
     * @param appAbility
     * @return int
     */
    int installApp(std::unique_ptr<AppAbility> appAbility);

    /**
     * @brief 卸载指定 ID 的 App
     *
     * @param appID
     * @return true
     * @return false
     */
    bool uninstallApp(int appID);

    /**
     * @brief 卸载所有 App
     *
     */
    void uninstallAllApps();

    /**
     * @brief 打开指定 ID 的 App
     *
     * @param appID
     * @return true
     * @return false
     */
    bool openApp(int appID);

    /**
     * @brief 关闭指定 ID 的 App
     *
     * @param appID
     * @return true
     * @return false
     */
    bool closeApp(int appID);

    /**
     * @brief 检查 App 是否存在
     *
     * @param appID
     * @return true
     * @return false
     */
    bool isAppExist(int appID);

    /**
     * @brief 获取当前 App 数量
     *
     * @return std::size_t
     */
    std::size_t getAppNum();

    /**
     * @brief 获取指定 ID 的 App 信息
     *
     * @param appID
     * @return AppAbility::AppInfo_t
     */
    AppAbility::AppInfo_t getAppInfo(int appID);

    /**
     * @brief 获取所有 App 的 App 信息
     *
     * @return std::vector<AppAbility::AppInfo_t>
     */
    std::vector<AppAbility::AppInfo_t> getAllAppInfo();

    /**
     * @brief 获取指定 ID 的 App 当前生命周期状态
     *
     * @param appID
     * @return AppAbility::State_t
     */
    AppAbility::State_t getAppCurrentState(int appID);

    /* -------------------------------------------------------------------------- */
    /*                          Extension Ability Manager                         */
    /* -------------------------------------------------------------------------- */
    // 扩展管理器 API 封装。
    // 可以用来存放管理自己的各种派生 Ability

    /**
     * @brief 获取扩展管理器
     *
     * @return AbilityManager*
     */
    AbilityManager* extensionManager();

    /**
     * @brief 重置扩展管理器，销毁其中所有 Ability
     *
     */
    void resetExtensionManager();

    /**
     * @brief 在扩展管理器中创建 Ability，返回 Ability ID
     *
     * @param ability
     * @return int
     */
    int createExtension(std::unique_ptr<AbilityBase> ability)
    {
        return extensionManager()->createAbility(std::move(ability));
    }

    /**
     * @brief 在扩展管理器中销毁指定 ID 的 Ability
     *
     * @param abilityID
     * @return true
     * @return false
     */
    bool destroyExtension(int abilityID)
    {
        return extensionManager()->destroyAbility(abilityID);
    }

    /**
     * @brief 获取扩展管理器中指定 ID Ability 的实例原始指针，以便外部调用 API，最不安全的一集
     *
     * @tparam T 扩展 Ability 类型
     * @param abilityID Ability ID
     * @return T
     */
    template <typename T>
    T* getExtensionInstance(int abilityID)
    {
        return static_cast<T*>(extensionManager()->getAbilityInstance(abilityID));
    }

private:
    std::unique_ptr<AbilityManager> _app_ability_manager;
    std::unique_ptr<AbilityManager> _extension_ability_manager;

    // 懒加载
    AbilityManager* get_app_ability_manager();
    AbilityManager* get_extension_ability_manager();
};

/* -------------------------------------------------------------------------- */
/*                                  Singleton                                 */
/* -------------------------------------------------------------------------- */
// 全局单例

/**
 * @brief 获取 Mooncake 单例
 *
 * @return Mooncake&
 */
Mooncake& GetMooncake();

/**
 * @brief 销毁 Mooncake 单例
 *
 */
void DestroyMooncake();

} // namespace mooncake
