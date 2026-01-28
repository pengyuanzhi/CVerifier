# CVerifier 插件 API 文档

| 文档版本 | 日期 | 作者 | 变更说明 |
|---------|------|------|---------|
| 0.1.0 | 2025-01-28 | CVerifier Team | 初始版本 |

---

## 目录

- [1. 概述](#1-概述)
- [2. 插件接口](#2-插件接口)
  - [2.1 IPlugin 接口](#21-iplugin-接口)
  - [2.2 生命周期管理](#22-生命周期管理)
  - [2.3 验证函数](#23-验证函数)
- [3. 插件开发](#3-插件开发)
  - [3.1 创建插件项目](#31-创建插件项目)
  - [3.2 实现插件类](#32-实现插件类)
  - [3.3 导出插件](#33-导出插件)
  - [3.4 编译插件](#34-编译插件)
- [4. 插件配置](#4-插件配置)
  - [4.1 配置文件](#41-配置文件)
  - [4.2 加载插件](#42-加载插件)
- [5. 插件示例](#5-插件示例)
  - [5.1 简单检查器](#51-简单检查器)
  - [5.2 高级检查器](#52-高级检查器)
- [6. 插件沙箱](#6-插件沙箱)
  - [6.1 资源限制](#61-资源限制)
  - [6.2 进程隔离](#62-进程隔离)
  - [6.3 异常处理](#63-异常处理)
- [7. 最佳实践](#7-最佳实践)

---

## 1. 概述

CVerifier 支持通过插件系统扩展其功能。插件是动态加载的共享库（.so/.dll），可以实现自定义的漏洞检测器、分析器或报告生成器。

### 1.1 插件类型

1. **漏洞检测器**：检测特定类型的漏洞
2. **分析器**：执行特殊分析（如数据流分析）
3. **转换器**：转换中间表示
4. **报告器**：生成自定义报告格式

### 1.2 插件优势

- **扩展性**：无需修改核心代码即可添加新功能
- **模块化**：每个插件独立开发和维护
- **可重用**：插件可以在不同项目中重用
- **隔离性**：插件崩溃不影响主程序

---

## 2. 插件接口

### 2.1 IPlugin 接口

所有插件必须实现 `IPlugin` 接口：

```cpp
// include/cverifier/IPlugin.h

namespace cverifier {

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 插件信息
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;

    // 生命周期管理
    virtual void initialize(const PluginConfig& config) = 0;
    virtual void shutdown() = 0;

    // 验证函数
    virtual bool verify(LLIRFunction* function) = 0;

    // 报告生成
    virtual std::vector<VulnerabilityReport> getReports() const = 0;

    // 健康检查（可选）
    virtual bool isHealthy() const {
        return true;
    }
};

} // namespace cverifier
```

### 2.2 生命周期管理

插件的生命周期：

```
加载 → 创建实例 → 初始化 → 验证 → 获取报告 → 关闭 → 销毁
```

### 2.3 验证函数

`verify()` 是插件的核心函数：

```cpp
bool verify(LLIRFunction* function);
```

**参数**：
- `function`：待验证的 LLIR 函数

**返回值**：
- `true`：验证通过（未发现漏洞）
- `false`：发现漏洞（通过 `getReports()` 获取）

---

## 3. 插件开发

### 3.1 创建插件项目

#### 目录结构

```
my_plugin/
├── CMakeLists.txt
├── MyPlugin.h
├── MyPlugin.cpp
└── README.md
```

#### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)

# 插件名称
set(PLUGIN_NAME MyPlugin)

# 创建共享库
add_library(${PLUGIN_NAME} SHARED
    MyPlugin.cpp
)

# 设置输出名称（无 lib 前缀）
set_target_properties(${PLUGIN_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${PLUGIN_NAME}
)

# 包含目录
target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CVERIFIER_ROOT}/include
)

# 链接库
target_link_libraries(${PLUGIN_NAME} PRIVATE
    cverifier-core
)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PLUGIN_NAME} PRIVATE
        -fPIC
        -fvisibility=hidden
    )
endif()

# 安装
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION lib/cverifier/plugins
)
```

### 3.2 实现插件类

#### MyPlugin.h

```cpp
#ifndef MY_PLUGIN_H
#define MY_PLUGIN_H

#include "cverifier/IPlugin.h"
#include <string>
#include <vector>

namespace cverifier {

class MyPlugin : public IPlugin {
public:
    MyPlugin();
    ~MyPlugin() override;

    // IPlugin 接口
    std::string getName() const override {
        return "MyPlugin";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    std::string getDescription() const override {
        return "My custom vulnerability checker";
    }

    void initialize(const PluginConfig& config) override;
    bool verify(LLIRFunction* function) override;
    std::vector<VulnerabilityReport> getReports() const override;
    void shutdown() override;

private:
    PluginConfig config_;
    std::vector<VulnerabilityReport> reports_;
    bool initialized_;
};

} // namespace cverifier

#endif
```

#### MyPlugin.cpp

```cpp
#include "MyPlugin.h"
#include "cverifier/Core.h"

namespace cverifier {

MyPlugin::MyPlugin() : initialized_(false) {}

MyPlugin::~MyPlugin() {
    shutdown();
}

void MyPlugin::initialize(const PluginConfig& config) {
    // 加载配置
    config_ = config;

    // 执行初始化
    // ...

    initialized_ = true;
}

bool MyPlugin::verify(LLIRFunction* function) {
    if (!initialized_) {
        return false;
    }

    reports_.clear();

    // 执行验证逻辑
    // ...

    return reports_.empty();
}

std::vector<VulnerabilityReport> MyPlugin::getReports() const {
    return reports_;
}

void MyPlugin::shutdown() {
    if (initialized_) {
        reports_.clear();
        initialized_ = false;
    }
}

} // namespace cverifier
```

### 3.3 导出插件

插件必须导出两个函数：

```cpp
extern "C" {

/**
 * 创建插件实例
 */
cverifier::IPlugin* createPlugin() {
    return new cverifier::MyPlugin();
}

/**
 * 销毁插件实例
 */
void destroyPlugin(cverifier::IPlugin* plugin) {
    delete plugin;
}

} // extern "C"
```

### 3.4 编译插件

```bash
cd my_plugin
mkdir build && cd build
cmake -DCVERIFIER_ROOT=/path/to/cverifier ..
make
```

生成 `MyPlugin.so`（Linux）或 `MyPlugin.dll`（Windows）

---

## 4. 插件配置

### 4.1 配置文件

插件配置文件（YAML）：

```yaml
# plugins.yaml

plugins:
  - name: MyPlugin
    path: /usr/lib/cverifier/plugins/MyPlugin.so
    enabled: true
    priority: 10
    config:
      check-depth: 100
      timeout: 300

  - name: AnotherPlugin
    path: /home/user/.cverifier/plugins/AnotherPlugin.so
    enabled: true
    priority: 5
```

### 4.2 加载插件

CVerifier 自动加载插件：

```bash
cverifier --plugin-config=plugins.yaml source.c
```

或从目录加载所有插件：

```bash
cverifier --plugin-dir=/usr/lib/cverifier/plugins source.c
```

---

## 5. 插件示例

### 5.1 简单检查器

检测未初始化变量：

```cpp
bool UninitVarChecker::verify(LLIRFunction* function) {
    for (auto& bb : function->getBasicBlocks()) {
        for (auto& inst : bb->getInstructions()) {
            if (inst->getOpcode() == Instruction::Load) {
                auto* loadInst = static_cast<LoadInst*>(inst.get());

                // 检查操作数是否已初始化
                if (!isInitialized(loadInst->getOperand(0))) {
                    VulnerabilityReport report;
                    report.type = "uninitialized-variable";
                    report.severity = Severity::Warning;
                    report.location = inst->getLocation();
                    report.message = "Potential use of uninitialized variable";

                    reports_.push_back(report);
                }
            }
        }
    }

    return reports_.empty();
}
```

### 5.2 高级检查器

检测浮点累积误差：

```cpp
bool FloatAccumChecker::verify(LLIRFunction* function) {
    for (auto& bb : function->getBasicBlocks()) {
        if (!bb->isLoopHeader()) {
            continue;
        }

        for (auto& inst : bb->getInstructions()) {
            if (inst->getOpcode() == Instruction::FAdd) {
                // 检测循环中的浮点加法
                VulnerabilityReport report;
                report.type = "float-accumulation";
                report.severity = Severity::Warning;
                report.location = inst->getLocation();
                report.message = "Potential float accumulation error in loop";

                // 添加修复建议
                report.fixSuggestions.push_back(
                    "Consider using Kahan summation"
                );

                reports_.push_back(report);
            }
        }
    }

    return reports_.empty();
}
```

---

## 6. 插件沙箱

### 6.1 资源限制

```cpp
struct ResourceLimits {
    size_t maxMemoryMB = 512;
    int maxCPUTimeSec = 60;
    int maxThreads = 4;
};

// 在沙箱中执行
auto result = sandbox.executeInSandbox(
    plugin, function, ResourceLimits{}
);
```

### 6.2 进程隔离

Unix/Linux 使用 `fork`：

```cpp
pid_t pid = fork();

if (pid == 0) {
    // 子进程：执行插件
    plugin->verify(function);
    _exit(0);
} else {
    // 父进程：等待结果
    int status;
    waitpid(pid, &status, 0);
}
```

Windows 使用 Job Object：

```cpp
HANDLE job = CreateJobObject(NULL, NULL);

JOBOBJECT_BASIC_LIMIT_INFORMATION limits;
limits.PerJobUserTimeLimit = 60 * 10000000;  // 60 秒

SetInformationJobObject(job, JobObjectBasicLimitInformation,
                       &limits, sizeof(limits));
```

### 6.3 异常处理

```cpp
try {
    plugin->verify(function);
} catch (const std::exception& e) {
    logger.error("Plugin crashed: " + std::string(e.what()));

    // 记录错误，但不影响主程序
    reports_.clear();
}
```

---

## 7. 最佳实践

### 7.1 性能优化

1. **缓存结果**：避免重复计算
2. **惰性求值**：只在需要时计算
3. **并行处理**：利用多线程

### 7.2 错误处理

1. **验证输入**：检查参数有效性
2. **异常安全**：使用 RAII
3. **日志记录**：记录错误信息

### 7.3 代码质量

1. **遵循 C++17 标准**
2. **使用智能指针**：避免内存泄漏
3. **单元测试**：测试插件功能

### 7.4 文档

1. **README.md**：插件说明
2. **API 文档**：如果有自定义 API
3. **示例**：使用示例

---

## 附录 A: 内置插件

CVerifier 提供以下内置插件：

1. **BufferOverflowPlugin**：缓冲区溢出检测
2. **NullPointerPlugin**：空指针解引用检测
3. **MemoryLeakPlugin**：内存泄漏检测
4. **IntegerOverflowPlugin**：整数溢出检测
5. **FloatOverflowPlugin**：浮点溢出检测
6. **DivisionByZeroPlugin**：除零错误检测

## 附录 B: 插件调试

启用插件调试：

```bash
cverifier --verbose --plugin-debug source.c
```

查看插件日志：

```bash
cverifier --log-level=DEBUG source.c
```

## 附录 C: 相关文档

- [架构设计文档](architecture.md) - 第 3.7 节：插件系统
- [详细设计文档](design.md) - 第 7 节：插件系统设计
- [规约语言参考](spec-language.md) - 规约编写指南

---

**文档结束**
