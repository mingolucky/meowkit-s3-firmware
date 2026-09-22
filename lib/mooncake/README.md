# Mooncake

适用于单片机的多 App 管理调度框架

A multi-app management and scheduling framework designed for mcu

------

- 结构化 App 生命周期设计
- 多 App 管理
- 灵活的生命周期行为抽象

单个 App：

```cpp
// 派生一个 App
class MyApp : public AppAbility {
public:
    // 重写生命周期回调函数
    void onOpen() override
    {
        printf("[MyApp] on open\n");
    }
    void onRunning() override
    {
        printf("[MyApp] on running\n");
    }
};

Mooncake mc;

// 安装 App
auto my_app_id = mc.installApp(std::make_unique<MyApp>());

// 打开 App
mc.openApp(my_app_id);

// 保持 Mooncake 更新
while (1) {
    mc.update();
}
// 输出：
// [MyApp] on open
// [MyApp] on running
// [MyApp] on running
// [MyApp] on running
// [MyApp] on running
// [MyApp] on running
// ...
```

多个 App，由事件切换当前运行的 App：

```cpp
// App A
class MyAppA : public AppAbility {
public:
    void onRunning() override { /* ... */  }
};

// App B
class MyAppB : public AppAbility {
public:
    void onRunning() override { /* ... */  }
};

Mooncake mc;

// 安装 App
auto my_app_id_a = mc.installApp(std::make_unique<MyAppA>());
auto my_app_id_b = mc.installApp(std::make_unique<MyAppB>());

while (1) {
    // 事件 A 时：关闭 App B，打开 App A
    if (事件A) {
        mc.closeApp(my_app_id_b);
        mc.openApp(my_app_id_a);
    }

    // 事件 B 时：关闭 App A，打开 App B
    else if (事件B) {
        mc.closeApp(my_app_id_a);
        mc.openApp(my_app_id_b);
    }

    // 更新 Mooncake
    mc.update();
}
```

全局单例：

```cpp
GetMooncake().installApp(std::make_unique<MyAppA>());
GetMooncake().installApp(std::make_unique<MyAppB>());

DestroyMooncake();
```

App 具体生命周期[图表](https://github.com/Forairaaaaa/mooncake?tab=readme-ov-file#appability)

更多使用案例可以参考 [例程](https://github.com/Forairaaaaa/mooncake/tree/main/example) 和 [Mooncake 工程模板](https://github.com/Forairaaaaa/mooncake_app_boilerplate)

## API

常用管理接口：

### 安装

```cpp
/**
 * @brief 安装 App，返回 App ID
 *
 * @param appAbility
 * @return int
 */
int installApp(std::unique_ptr<AppAbility> appAbility);
```

### 卸载
```cpp
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
```

### 打开、关闭

```cpp
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
```

### App 信息
```cpp
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
```

### 更新

App 的所有生命周期回调都会集中在 Mooncake 的  `update()` 方法中触发：

```cpp
/**
 * @brief 更新 Mooncake，刷新 App 及 Extension 状态，触发生命周期回调
 *
 */
void update();
```

一般可以丢进 `while` 里

### 线程安全性

所有 API 都为线程 **不安全**，Mooncake 只提供管理器和一个集中的 `update()` 调度接口

如果需要线程安全，确保 Mooncake 在临界区：

```cpp
// 一边
while (1) {
    mutex.lock();
    mc.update();
    mutex.unlock();
}

// 另一边
mutex.lock();
mc.openApp(114514);
mutex.unlock();
```

## Ability 模型

这概念抄[鸿蒙](https://docs.openharmony.cn/pages/v4.1/zh-cn/application-dev/application-models/abilitykit-overview.md)的，有点像超级青春版

**Ability** 为一个应用的行为抽象，每个 Ability 都会有最基础的 **创建、运行、销毁** 三个状态

基于这三个状态，可以派生出不同生命周期的 Ability 类型

```mermaid
classDiagram
    AbilityBase <|-- BasicAbility
    AbilityBase <|-- UIAbility
    AbilityBase <|-- WorkerAbility
    AbilityBase <|-- AppAbility
    class AbilityBase{
        baseCreate()
        baseUpdate()
        baseDestroy()
    }
    class BasicAbility{
        onCreate()
        onRunning()
        onDestroy()
    }
    class UIAbility{
        onCreate()
        onShow()
        onForeground()
        onBackground()
        onHide()
        onDestroy()
    }
    class WorkerAbility{
        onCreate()
        onResume()
        onRunning()
       	onPause()
        onDestroy()
    }
    class AppAbility{
        onCreate()
        onOpen()
        onRunning()
        onSleeping()
        onClose()
        onDestroy()
    }
```

### BasicAbility

 `BasicAbility` 类型生命周期：

```mermaid
graph TD;
    onCreate-->onRunning;
    onRunning-- destroy -->onDestroy;
    
```

提供了最简单的三段式生命周期

可以把 `onCreate` 比作 **Arduino** 的 `setup`，`onRunning` 比作 `loop` ：

```cpp
class ShitPrinter : public BasicAbility {
public:
    void onCreate() override
    {
        Serial.begin(114514);
    }
    void onRunning() override
    {
        Serial.println("shit");
    }
};
```

### UIAbility

 `UIAbility` 类型生命周期：

```mermaid
graph TD;
    onCreate-->onForeground;
    onForeground-- hide -->onHide;
    onHide-->onBackground;
    onBackground-- show -->onShow;
    onShow-->onForeground;
    onForeground-- destroy -->onDestroy;
    onBackground-- destroy -->onDestroy;
    
```

扩展出前后台概念，创建之后默认为前台状态，可由 `hide` 和 `show` 接口切换

适合具有 前台/后台、获焦/失焦、可见/不可见 等概念的 UI 行为：

```cpp
class 小黑子按钮 : public UIAbility {
public:
    // 鼠标悬浮时
    void onForeground() override
    {
        // 铁山靠
    }
    // 鼠标不悬浮时
    void onBackground() override
    {
        // ctrl
    }
};
```

### WorkerAbility

 `WorkerAbility` 类型生命周期：

```mermaid
graph TD;
    onCreate-->onRunning;
    onRunning-- pause -->onPause;
    onPause-->暂停;
    暂停-- resume -->onResume;
    onResume-->onRunning;
    onRunning-- destroy -->onDestroy;
    暂停-- destroy -->onDestroy;
    
```

和 `BasicAbility` 差不多，只是扩展出暂停和恢复概念，创建之后默认为运行状态

可由 `pause` 和 `resume` 接口切换，暂停时不会触发任何回调

适合由其他 Ability 主导的并行行为：

```cpp
class ShitPrinter可暂停版 : public WorkerAbility {
public:
    void onCreate() override
    {
        Serial.begin(114514);
    }
    void onRunning() override
    {
        Serial.println("shit");
    }
    void onPause() override
    {
        Serial.println("我说婷婷");
    }
    void onResume() override
    {
        Serial.println("又来力");
    }
};

class JustMakeMoney : public WorkerAbility {
public:
    void onRunning() override
    { 
        // 去洗衣厂做饭
    }
};
```

### AppAbility

 `AppAbility` 类型生命周期：

```mermaid
graph TD;
    onCreate-->onSleeping;
    onOpen-->onRunning;
    onRunning-- close -->onClose;
    onClose-->onSleeping;
    onSleeping-- open -->onOpen;
    onRunning-- destroy -->onDestroy;
    onSleeping-- destroy -->onDestroy;
    
```

扩展出打开和关闭概念，创建后默认为关闭状态（sleeping），可由 `open` 和 `close` 接口切换状态

Mooncake 框架所管理的 App 就是基于 `AppAbility` 类型

所以只要继承 `AppAbility` 类型，重写生命周期回调，就可以实现我们自己的 App 了：

```cpp
// 一个
class MyApp_1 : public AppAbility {
public:
    void onOpen() override
    {
        // 上号
    }
    void onRunning() override
    {
        // 大乱斗
    }
    void onClose() override
    {
        // 耻辱下播
    }
};

// 另一个
class MyApp_2 : public AppAbility {
public:
    void onOpen() override
    {
        // 去自习室
    }
    void onRunning() override
    {
        // 钓鱼
    }
    void onClose() override
    {
        // 回宿舍
    }
};
```

还提供基础的 App 信息接口：

```cpp
struct AppInfo_t {
    // 名称
    std::string name;
    // 图标
    void* icon = nullptr;
    // ...
    void* userData = nullptr;
};
```

## Mooncake

**Mooncake** 内部提供两个 **Ability** 管理器：

```cpp
std::unique_ptr<AbilityManager> _app_ability_manager;
std::unique_ptr<AbilityManager> _extension_ability_manager;
```

- **App Ability 管理器** 只用于管理  `AppAbility` 类型，并提供了针对性的接口封装

- **Extension Ability 管理器** 可以用于管理任意 Ability 类型，比如需要一些 `WorkerAbility` 来监听数据、`UIAbility` 渲染图像、自己派生的 `CustomAbility` 等都可以丢到这里统一由 Mooncake 管理

比如：

```cpp
// 安装 App
mc.installApp(std::make_unique<MyApp>());

// 安装扩展
mc.createExtension(std::make_unique<MyWorker>());
mc.createExtension(std::make_unique<MyUI>());

// 更新 Mooncake
for (int i = 0; i < 3; i++) {
    mc.update();
}
// 输出：
// [worker] on running
// [ui] on foreground
// [app] on running
// [worker] on running
// [ui] on foreground
// [app] on running
// [worker] on running
// [ui] on foreground
```

### AbilityManager

**Ability 管理器** 负责 Ability 实例的创建、储存和销毁，以及原始生命周期的调度触发

每个 Ability 实例会分配一个 **Ability ID**，方便查找索引：

```cpp
/**
 * @brief 创建 Ability，返回 Ability ID
 *
 * @param ability
 * @return int
 */
int createAbility(std::unique_ptr<AbilityBase> ability);

/**
 * @brief 销毁指定 ID 的 Ability
 *
 * @param abilityID
 * @return true
 * @return false
 */
bool destroyAbility(int abilityID);
```

**Ability ID** 并不是绝对唯一的，当一个 Ability 被销毁时，对应的 ID 会被 **回收**，等待分配到新的 Ability

AbilityManager 还提供针对性的操作接口，简化常用的派生 Ability 的状态切换，避免直接的实例操作：

```cpp
/* -------------------------------------------------------------------------- */
/*                            Ability API Wrapping                            */
/* -------------------------------------------------------------------------- */
// 对外暴露针对性的 Ability 操作接口

bool showUIAbility(int abilityID);
bool hideUIAbility(int abilityID);
UIAbility::State_t getUIAbilityCurrentState(int abilityID);

bool pauseWorkerAbility(int abilityID);
bool resumeWorkerAbility(int abilityID);
WorkerAbility::State_t getWorkerAbilityCurrentState(int abilityID);

bool openAppAbility(int abilityID);
bool closeAppAbility(int abilityID);
AppAbility::AppInfo_t getAppAbilityAppInfo(int abilityID);
AppAbility::State_t getAppAbilityCurrentState(int abilityID);
```

