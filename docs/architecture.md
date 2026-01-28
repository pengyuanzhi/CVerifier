# CVerifier 软件架构设计文档

| 文档版本 | 日期 | 作者 | 变更说明 |
|---------|------|------|---------|
| 0.1.0 | 2025-01-28 | CVerifier Team | 初始版本 |

---

## 目录

- [1. 架构概述](#1-架构概述)
  - [1.1 架构目标](#11-架构目标)
  - [1.2 架构原则](#12-架构原则)
  - [1.3 架构风格](#13-架构风格)
- [2. 系统架构](#2-系统架构)
  - [2.1 八层架构设计](#21-八层架构设计)
  - [2.2 架构图](#22-架构图)
  - [2.3 架构说明](#23-架构说明)
  - [2.4 数据流](#24-数据流)
- [3. 核心组件](#3-核心组件)
  - [3.1 前端处理层](#31-前端处理层)
  - [3.2 中间表示层](#32-中间表示层)
  - [3.3 核心分析层](#33-核心分析层)
  - [3.4 基础设施层](#34-基础设施层)
  - [3.5 用户接口层](#35-用户接口层)
    - [3.5.1 CLI 工具](#351-cli-工具)
    - [3.5.2 Web UI](#352-web-ui-新增)
  - [3.6 规约层](#36-规约层-新增)
    - [3.6.1 ACSL注解解析器](#361-acsl注解解析器)
    - [3.6.2 验证函数库 (C API)](#362-验证函数库-c-api)
    - [3.6.3 规约代码隔离](#363-规约代码隔离)
  - [3.7 插件系统](#37-插件系统-新增)
    - [3.7.1 插件接口](#371-插件接口)
    - [3.7.2 插件加载器](#372-插件加载器)
    - [3.7.3 插件配置](#373-插件配置)
    - [3.7.4 插件沙箱](#374-插件沙箱)
- [4. 技术选型](#4-技术选型)
  - [4.1 编程语言](#41-编程语言)
  - [4.2 框架选择](#42-框架选择)
    - [4.2.1 LLVM/Clang 15+](#421-llvmclang-15)
    - [4.2.2 CMake 3.20+](#422-cmake-320)
    - [4.2.3 React 18+](#423-react-18-新增)
    - [4.2.4 TypeScript](#424-typescript-新增)
    - [4.2.5 Crow](#425-crow-新增)
    - [4.2.6 WebSocket++](#426-websocket-新增)
    - [4.2.7 nlohmann/json](#427-nlohmannjson-新增)
  - [4.3 依赖库](#43-依赖库)
  - [4.4 选型理由](#44-选型理由)
- [5. 架构决策记录](#5-架构决策记录)
  - [5.1 为什么选择 LLVM/Clang？](#51-为什么选择-llvmclang)
  - [5.2 为什么选择 Z3？](#52-为什么选择-z3)
  - [5.3 为什么使用自定义 LLIR？](#53-为什么使用自定义-llir)
  - [5.4 为什么采用混合分析策略？](#54-为什么采用混合分析策略)
  - [5.5 为什么实现规约分离？](#55-为什么实现规约分离-新增)
  - [5.6 为什么规约使用C语言？](#56-为什么规约使用c语言-新增)
  - [5.7 为什么添加Web UI？](#57-为什么添加web-ui-新增)
  - [5.8 为什么选择React而不是Vue/Angular？](#58-为什么选择react而不是vueangular-新增)
  - [5.9 为什么选择Crow框架？](#59-为什么选择crow框架-新增)
- [6. 部署架构](#6-部署架构)
  - [6.1 单机部署](#61-单机部署)
  - [6.2 容器部署](#62-容器部署)
  - [6.3 云端部署（可选）](#63-云端部署可选)
- [7. 质量属性](#7-质量属性)
  - [7.1 性能优化策略](#71-性能优化策略)
  - [7.2 可扩展性设计](#72-可扩展性设计)
  - [7.3 容错设计](#73-容错设计)
- [8. 安全架构](#8-安全架构)
  - [8.1 静态分析保证](#81-静态分析保证)
  - [8.2 数据隐私保护](#82-数据隐私保护)
  - [8.3 输入验证](#83-输入验证)

---

## 1. 架构概述

### 1.1 架构目标

CVerifier 的架构设计旨在实现以下目标：

1. **高性能**：通过并行分析、状态合并、约束缓存等技术实现高性能
2. **高准确率**：结合符号执行和抽象解释，降低误报和漏报
3. **可扩展性**：模块化设计，易于添加新的漏洞检测器和抽象域
4. **可维护性**：清晰的模块边界，良好的代码组织
5. **工业质量**：健壮的错误处理，完整的日志和监控

### 1.2 架构原则

#### 1.2.1 关注点分离

**原则**：将系统划分为多个独立的模块，每个模块负责特定的功能

**应用**：
- 前端只负责代码解析
- 分析器专注于漏洞检测
- 报告器只负责格式化输出

#### 1.2.2 依赖倒置

**原则**：高层模块不依赖低层模块，两者都依赖抽象

**应用**：
- 分析器依赖 IParser 接口，而不是 ClangParser
- 检测器依赖 ISMTSolver 接口，而不是 Z3Solver

#### 1.2.3 单一职责

**原则**：每个类和模块只有一个改变的理由

**应用**：
- BufferOverflowChecker 只负责缓冲区溢出检测
- PathConstraint 只负责路径约束管理

#### 1.2.4 开闭原则

**原则**：对扩展开放，对修改关闭

**应用**：
- 通过 IVulnerabilityChecker 接口添加新检测器
- 通过 IAbstractDomain 接口添加新抽象域

#### 1.2.5 接口隔离

**原则**：客户端不应该依赖它不需要的接口

**应用**：
- ISMTSolver 只定义必要的求解方法
- IParser 只定义必要的方法

### 1.3 架构风格

CVerifier 采用 **分层架构** 和 **管道-过滤器架构** 的混合风格：

#### 1.3.1 分层架构

```
用户接口层 (CLI)
    ↓
分析配置层
    ↓
核心分析层
    ↓
中间表示层
    ↓
基础设施层
    ↓
前端处理层
```

**优势**：
- 清晰的依赖关系
- 易于理解和维护
- 支持并行开发

#### 1.3.2 管道-过滤器

```
源代码 → 解析器 → IR转换 → 分析器 → 检测器 → 报告生成
```

**优势**：
- 数据流清晰
- 每个阶段独立
- 易于测试和调试

---

## 2. 系统架构

### 2.1 八层架构设计

CVerifier 采用八层架构，从上到下依次为：

1. **用户接口层**：提供 CLI 工具、IDE 插件、Web UI
2. **分析配置层**：配置管理、检查规则、报告生成
3. **核心分析层**：符号执行引擎、抽象解释器、混合分析器
4. **中间表示层**：IR 转换、IR 优化、CFG 构建
5. **基础设施层**：SMT 求解器、约束求解器、定理证明器
6. **规约层**：规约解析器、验证函数库、规约代码隔离 ⭐新增
7. **插件系统**：插件加载器、插件接口、插件沙箱 ⭐新增
8. **前端处理层**：预处理器、C Parser、AST 构建

### 2.2 架构图

```
┌───────────────────────────────────────────────────────────────┐
│                      用户接口层                                │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────────┐     │
│  │ CLI 工具  │  │ IDE 插件 │  │  Web UI (React前端)      │     │
│  └──────────┘  └──────────┘  │  - 任务管理               │     │
│                              │  - 代码编辑器 (Monaco)    │     │
│                              │  - 实时进度监控           │     │
│                              │  - 结果可视化             │     │
│                              └──────────────────────────┘     │
└───────────────────────────────────────────────────────────────┘
                                   ↓ REST/WebSocket
┌───────────────────────────────────────────────────────────────┐
│                   Web 服务层 (Crow框架) ⭐新增                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                    │
│  │ REST API │  │WebSocket │  │ 任务调度器 │                    │
│  └──────────┘  └──────────┘  └──────────┘                    │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                     分析配置层                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                    │
│  │配置管理器 │  │ 检查规则 │  │ 报告生成 │                    │
│  └──────────┘  └──────────┘  └──────────┘                    │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                     核心分析层                                 │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐              │
│  │ 符号执行引擎 │  │ 抽象解释器 │  │ 混合分析器 │              │
│  └────────────┘  └────────────┘  └────────────┘              │
│  ┌────────────────────────────────────────────────────────┐   │
│  │              漏洞检测器                                   │   │
│  │ Buffer │ NullPtr │ MemLeak │ IntOverflow │ Float ⭐   │   │
│  │ Overflow│        │        │             │ DivideBy0 ⭐│   │
│  └────────────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                     中间表示层                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                    │
│  │ IR 转换   │  │ IR 优化   │  │ CFG 构建  │                    │
│  └──────────┘  └──────────┘  └──────────┘                    │
│  ┌────────────────────────────────────────────────────────┐   │
│  │              LLIR (轻量级中间表示)                        │   │
│  └────────────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                     基础设施层                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                    │
│  │ Z3 求解器 │  │ 约束构建器 │  │ 模型提取器 │                    │
│  └──────────┘  └──────────┘  └──────────┘                    │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                      规约层 ⭐新增                              │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────┐        │
│  │ACSL解析器   │  │验证函数库   │  │ 规约代码隔离      │        │
│  │/*@..*/     │  │ (C API)    │  │ - 独立编译        │        │
│  └────────────┘  └────────────┘  │ - 命名空间隔离    │        │
│                                 │ - 沙箱执行        │        │
│                                 └──────────────────┘        │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                    插件系统 ⭐新增                              │
│  ┌────────────┐  ┌────────────┐  ┌──────────────────┐        │
│  │ 插件加载器   │  │ 插件接口    │  │ 插件沙箱          │        │
│  │ .so/.dll   │  │ IPlugin    │  │ - 进程隔离        │        │
│  └────────────┘  └────────────┘  │ - 资源限制        │        │
│                                 └──────────────────┘        │
└───────────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────────────┐
│                     前端处理层                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                    │
│  │ 预处理器  │  │ Clang    │  │ AST 构建 │                    │
│  │          │  │ Parser   │  │          │                    │
│  └──────────┘  └──────────┘  └──────────┘                    │
└───────────────────────────────────────────────────────────────┘
```

**架构说明**：
- **工具主体**: C++17 实现
- **规约API**: C语言实现，独立编译为共享库
- **插件**: 动态加载 .so/.dll 文件
- **Web UI**: React + TypeScript 前端，Crow C++ 后端

### 2.3 架构说明

#### 2.3.1 用户接口层

**职责**：提供用户与系统交互的接口

**组件**：
- **CLI 工具**：命令行工具 `cverifier`
- **IDE 插件**：VS Code、JetBrains 插件（可选）
- **Web UI**：Web 界面（可选，用于云端分析）

**接口**：
```cpp
int main(int argc, char** argv) {
    auto options = parseCommandLine(argc, argv);
    auto config = loadConfig(options.config);
    AnalysisManager analyzer(config);
    auto result = analyzer.analyze(options.sources);
    ReportGenerator::generate(result, options.format);
}
```

#### 2.3.2 分析配置层

**职责**：管理分析配置和检查规则

**组件**：
- **配置管理器**：加载和验证配置文件
- **检查规则**：定义检查器启用/禁用规则
- **报告生成**：格式化和输出分析结果

**配置格式**：YAML

```yaml
analysis:
  timeout: 300
  max-depth: 100
  abstraction:
    domain: interval

checkers:
  enable:
    - buffer-overflow
    - null-pointer
```

#### 2.3.3 核心分析层

**职责**：执行符号执行和抽象解释

**组件**：
- **符号执行引擎**：探索程序路径
- **抽象解释器**：快速近似程序行为
- **混合分析器**：结合两种方法
- **漏洞检测器**：检测特定类型的漏洞

**工作流程**：
1. 符号执行引擎探索程序路径
2. 抽象解释器剪枝不可行路径
3. 漏洞检测器在每个状态检查漏洞

#### 2.3.4 中间表示层

**职责**：管理中间表示

**组件**：
- **IR 转换**：LLVM IR → LLIR
- **IR 优化**：简化和优化 LLIR
- **CFG 构建**：构建控制流图

**LLIR 设计**：
- 轻量级，只包含必要的指令
- 易于符号执行的语义建模
- 保留源代码位置信息

#### 2.3.5 基础设施层

**职责**：提供 SMT 求解能力

**组件**：
- **Z3 求解器**：SMT 约束求解
- **约束构建器**：构建符号约束
- **模型提取器**：提取反例模型

#### 2.3.6 前端处理层

**职责**：解析 C 源代码

**组件**：
- **预处理器**：处理 #include、#define 等
- **Clang Parser**：词法和语法分析
- **AST 构建**：构建抽象语法树

### 2.4 数据流

```
C源文件 (.c)
    ↓
[预处理] → 预处理后的源代码
    ↓
[Clang解析] → AST
    ↓
[LLVM IR生成] → LLVM IR
    ↓
[IR转换] → LLIR
    ↓
[CFG构建] → 控制流图
    ↓
[数据流分析] → 数据流信息
    ↓
    ├→ [符号执行] → 符号状态 + 路径约束
    │                    ↓
    │                 [约束求解]
    │                    ↓
    └→ [抽象解释] → 抽象状态
                         ↓
                   [漏洞检测]
                         ↓
                   [漏洞列表]
                         ↓
                   [报告生成]
                         ↓
                   [分析报告]
```

---

## 3. 核心组件

### 3.1 前端处理层

#### 3.1.1 ClangParser

**职责**：使用 Clang LibTooling 解析 C 代码

**关键方法**：
```cpp
class ClangParser : public IParser {
public:
    std::unique_ptr<LLIRModule> parse(
        const std::string& sourceFile,
        const ParseOptions& options
    ) override;

    std::unique_ptr<LLIRModule> parseString(
        const std::string& sourceCode,
        const ParseOptions& options
    ) override;
};
```

**特点**：
- 支持完整的 C11/C17 标准
- 保留宏和预处理器信息
- 准确的行号和列号信息

#### 3.1.2 ASTBuilder

**职责**：构建 Clang AST

**特点**：
- 使用 RecursiveASTVisitor 遍历 AST
- 提取函数、变量、语句等信息

#### 3.1.3 IRConverter

**职责**：将 LLVM IR 转换为 LLIR

**转换规则**：
- 指令简化
- 基本块合并
- 保留 SSA 形式

### 3.2 中间表示层

#### 3.2.1 LLIR 模块

**核心类**：
- `LLIRModule`：顶级容器
- `LLIRFunction`：函数定义
- `LLIRBasicBlock`：基本块
- `LLIRInstruction`：指令

**指令类型**：
```cpp
enum class LLIRInstructionType {
    // 算术运算
    Add, Sub, Mul, Div, Rem,
    // 位运算
    And, Or, Xor, Shl, Shr,
    // 比较运算
    ICmp, FCmp,
    // 内存操作
    Alloca, Load, Store, GetElementPtr,
    // 控制流
    Br, Ret, Call,
    // 其他
    Phi, Select, Assert  // Assert 用于漏洞检测
};
```

#### 3.2.2 CFG 构建器

**职责**：构建控制流图

**功能**：
- 分析分支和跳转
- 计算支配树
- 计算控制依赖

### 3.3 核心分析层

#### 3.3.1 符号执行引擎

**核心类**：
```cpp
class SymbolicExecutionEngine : public ISymbolicExecutionEngine {
public:
    AnalysisResult analyze(
        LLIRFunction* function,
        SymbolicState* entryState = nullptr
    ) override;

    void setStrategy(ExplorationStrategy strategy) override;
    void setMaxDepth(int depth) override;
};
```

**路径探索策略**：
- **DFS**：深度优先，适合深路径
- **BFS**：广度优先，适合浅层 bug
- **Hybrid**：混合策略，平衡性能和覆盖率

**关键数据结构**：
- `SymbolicState`：符号状态
- `SymbolicStore`：符号存储
- `SymbolicHeap`：符号堆
- `PathConstraint`：路径约束

#### 3.3.2 抽象解释器

**核心类**：
```cpp
template<typename T>
class AbstractDomain {
public:
    virtual T bottom() = 0;
    virtual T top() = 0;
    virtual T join(T a, T b) = 0;
    virtual T widen(T a, T b) = 0;
    virtual T transfer(T state, LLIRInstruction* inst) = 0;
};
```

**抽象域层次**：
1. **常量域**：最精确
2. **区间域**：整数范围分析
3. **八边形域**：关系分析
4. **多面体域**：最精确但开销大

#### 3.3.3 漏洞检测器

**接口**：
```cpp
class IVulnerabilityChecker {
public:
    virtual std::vector<VulnerabilityReport> check(
        LLIRFunction* function
    ) = 0;

    virtual std::string getName() const = 0;
};
```

**实现**：
- `BufferOverflowChecker`：缓冲区溢出检测
- `NullPointerChecker`：空指针解引用检测
- `MemoryLeakChecker`：内存泄漏检测
- `IntegerOverflowChecker`：整数溢出检测

### 3.4 基础设施层

#### 3.4.1 SMT 求解器接口

**接口**：
```cpp
class ISMTSolver {
public:
    virtual bool check(const std::vector<Expr*>& constraints) = 0;
    virtual std::unordered_map<std::string, int64_t> getModel() = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
};
```

**Z3 实现**：
```cpp
class Z3Solver : public ISMTSolver {
public:
    Z3Solver();
    bool check(const std::vector<Expr*>& constraints) override;
    std::unordered_map<std::string, int64_t> getModel() override;
    void push() override;
    void pop() override;
};
```

### 3.5 用户接口层

#### 3.5.1 CLI 工具

**命令行选项**：
```bash
cverifier [options] <source-files>

选项：
  --config=<file>          # 配置文件
  --enable=<checkers>      # 启用检查器
  --disable=<checkers>     # 禁用检查器
  --output-format=<fmt>    # 输出格式
  --output=<file>          # 输出文件
  --timeout=<sec>          # 超时时间
  --max-depth=<n>          # 最大深度
  --verbose                # 详细输出
  --version                # 版本信息
  --help                   # 帮助信息
```

#### 3.5.2 Web UI ⭐新增

**前端架构**：
```
web-ui/
├── src/
│   ├── components/          # React组件
│   │   ├── TaskManager/     # 任务管理
│   │   ├── CodeEditor/      # Monaco编辑器
│   │   ├── ProgressMonitor/ # 进度监控
│   │   ├── ResultsView/     # 结果查看
│   │   └── VulnerabilityPanel/ # 漏洞面板
│   ├── services/            # API服务
│   │   ├── api.ts           # REST API封装
│   │   └── websocket.ts     # WebSocket封装
│   ├── pages/               # 页面
│   │   ├── Dashboard.tsx
│   │   ├── Analysis.tsx
│   │   └── Results.tsx
│   └── utils/               # 工具函数
├── public/
│   └── index.html
├── package.json
└── tsconfig.json
```

**后端架构**：
```cpp
// Web服务器 (C++ Crow框架)
class WebServer {
public:
    void start(int port);
    void stop();

private:
    // REST API端点
    void setupRoutes();
    void handleGetTasks(crow::request& req, crow::response& res);
    void handleCreateTask(crow::request& req, crow::response& res);
    void handleGetResults(crow::request& req, crow::response& res);

    // WebSocket端点
    void setupWebSocket();
    void broadcastProgress(const std::string& taskId, int progress);

    crow::SimpleApp app_;
    TaskScheduler& scheduler_;
};
```

**通信协议**：
- **REST API**: HTTP/JSON
- **实时通信**: WebSocket
- **认证**: JWT Token

### 3.6 规约层 ⭐新增

**职责**：解析和管理C语言编写的验证规约，实现规约与用户代码的分离

#### 3.6.1 ACSL注解解析器

**功能**：解析ACSL风格的注解（/*@ ... */）

**接口**：
```cpp
class SpecParser {
public:
    // 解析规约注解
    std::vector<Specification*> parse(const std::string& source);

    // 提取前置条件
    std::vector<Expr*> extractRequires(Function* func);

    // 提取后置条件
    std::vector<Expr*> extractEnsures(Function* func);

    // 提取循环不变量
    std::vector<Expr*> extractInvariant(Loop* loop);
};
```

**支持的ACSL语法**：
```c
/*@
  requires \valid_read(ptr);
  requires size > 0;
  requires \separated(ptr, other);
  assigns \nothing;
  ensures \result == 0 || \result == 1;
*/
int process(char* ptr, int size, char* other);
```

#### 3.6.2 验证函数库 (C API)

**功能**：提供预定义的C语言验证函数

**接口定义**：
```c
// verification.h - C语言验证API

// 内存验证
bool verifiable_read(void* ptr, int size);
bool verifiable_write(void* ptr, int size);
bool verifiable_pointer(void* ptr);
bool verifiable_separated(void* ptr1, void* ptr2, int size);

// 数值验证
bool in_range(int value, int min, int max);
bool is_positive(int value);
bool is_nonzero(int value);

// 断言函数
void assert_true(bool condition, const char* msg);
#define STATIC_ASSERT(cond) _Static_assert(cond, #cond)
```

**编译为共享库**：
```cmake
# CMakeLists.txt for spec library
add_library(verification SHARED
    src/verifiable_read.c
    src/verifiable_write.c
    src/assertions.c
)

target_include_directories(verification PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
```

#### 3.6.3 规约代码隔离

**隔离机制**：

1. **独立编译**：
   ```bash
   # 规约代码编译为独立共享库
   gcc -shared -fPIC -o libverification.so specs/*.c
   ```

2. **命名空间隔离**：
   ```c
   #ifdef VERIFICATION_SPEC
   // 规约代码只在此宏定义下编译
   int ver_buffer_check(char* ptr, int size) { ... }
   #endif
   ```

3. **沙箱执行**：
   ```cpp
   class SpecSandbox {
   public:
       // 在独立进程中执行规约代码
       VerificationResult executeInSandbox(
           const std::string& specCode,
           const std::vector<std::string>& args
       );
   };
   ```

**目录结构**：
```
project/
├── src/                    # 用户业务代码 (C/C++)
│   └── main.c
├── specs/                  # 规约代码 (C语言)
│   ├── buffer.spec.c
│   ├── memory.spec.c
│   └── libverification.so  # 编译后的规约库
└── cverifier               # 验证工具 (C++17)
```

### 3.7 插件系统 ⭐新增

**职责**：支持动态加载验证插件，扩展工具功能

#### 3.7.1 插件接口

**定义**：
```cpp
// IPlugin.h
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
    virtual bool verify(LLIRFunction* func) = 0;

    // 报告生成
    virtual std::vector<VulnerabilityReport> getReports() const = 0;
};
```

**插件示例**：
```cpp
// CustomChecker.cpp
class CustomChecker : public IPlugin {
public:
    std::string getName() const override {
        return "CustomChecker";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    void initialize(const PluginConfig& config) override {
        // 加载配置
    }

    bool verify(LLIRFunction* func) override {
        // 自定义验证逻辑
        return true;
    }

    std::vector<VulnerabilityReport> getReports() const override {
        return reports_;
    }

private:
    std::vector<VulnerabilityReport> reports_;
};

// 导出插件工厂函数
extern "C" {
    IPlugin* createPlugin() {
        return new CustomChecker();
    }

    void destroyPlugin(IPlugin* plugin) {
        delete plugin;
    }
}
```

#### 3.7.2 插件加载器

**功能**：动态加载.so/.dll文件

```cpp
class PluginLoader {
public:
    // 从目录加载所有插件
    void loadPluginsFromDirectory(const std::string& dir);

    // 加载单个插件
    std::shared_ptr<IPlugin> loadPlugin(const std::string& path);

    // 卸载插件
    void unloadPlugin(const std::string& name);

    // 获取所有插件
    std::vector<std::shared_ptr<IPlugin>> getAllPlugins();

private:
#ifdef _WIN32
    HMODULE loadLibrary(const std::string& path);
#else
    void* loadLibrary(const std::string& path);
#endif

    std::unordered_map<std::string, std::shared_ptr<IPlugin>> plugins_;
};
```

#### 3.7.3 插件配置

**配置文件格式**：
```yaml
# plugins.yaml
plugins:
  - name: BufferOverflowPlugin
    path: /usr/lib/cverifier/plugins/libbufferoverflow.so
    enabled: true
    priority: 10

  - name: FloatCheckerPlugin
    path: /usr/lib/cverifier/plugins/libfloatchecker.so
    enabled: true
    priority: 8
    config:
      check-precision-loss: true
      check-rounding-error: true

  - name: CustomPlugin
    path: /home/user/.cverifier/plugins/libcustom.so
    enabled: true
```

#### 3.7.4 插件沙箱

**安全机制**：

1. **进程隔离**：
   ```cpp
   class PluginSandbox {
   public:
       // 在独立进程中执行插件
       template<typename F>
       auto executeInSandbox(F&& func) -> decltype(func());
   };
   ```

2. **资源限制**：
   ```cpp
   struct ResourceLimits {
       size_t maxMemoryMB;     // 最大内存使用
       int maxCPUTimeSec;      // 最大CPU时间
       int maxThreads;         // 最大线程数
   };
   ```

3. **异常处理**：
   ```cpp
   try {
       plugin->verify(function);
   } catch (const std::exception& e) {
       // 记录错误，但不影响主程序
       logger.error("Plugin crashed: " + std::string(e.what()));
   }
   ```

---

## 4. 技术选型

### 4.1 编程语言

#### 4.1.1 C++17

**选择理由**：
1. **高性能**：接近 C 的性能，适合计算密集型任务
2. **丰富的库**：STL 提供容器、算法等
3. **面向对象**：支持良好的抽象和封装
4. **与 LLVM 兼容**：LLVM 本身是 C++ 编写的

**使用的 C++17 特性**：
- `std::optional`、`std::variant`
- 结构化绑定
- `if constexpr`
- 折叠表达式
- `std::filesystem`

### 4.2 框架选择

#### 4.2.1 LLVM/Clang 15+

**选择理由**：
1. **成熟的 C 前端**：支持完整的 C11/C17 标准
2. **LibTooling**：强大的 AST 操作能力
3. **LLVM IR**：高质量的中间表示
4. **活跃的社区**：持续维护和更新
5. **工业应用**：被广泛使用（Clang、Swift、Rust）

**集成方式**：
```cmake
find_package(LLVM 15 REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
target_link_libraries(cverifier ${LLVM_LIBS})
```

#### 4.2.2 CMake 3.20+

**选择理由**：
1. **跨平台**：支持 Linux、macOS、Windows
2. **LLVM 集成**：LLVM 使用 CMake
3. **广泛使用**：事实上的标准

#### 4.2.3 React 18+ ⭐新增

**选择理由**：
1. **组件化架构**：易于构建复杂的 UI
2. **丰富的生态系统**：Ant Design, Material-UI
3. **TypeScript 支持**：类型安全
4. **虚拟 DOM**：高性能渲染
5. **大型社区**：大量的库和工具

**用于**：Web UI 前端

**核心库**：
- `react` + `react-dom`
- `@types/react` + `@types/react-dom`
- `antd` (Ant Design) 或 `@mui/material`
- `axios` (HTTP 客户端)
- `@monaco-editor/react` (代码编辑器)

#### 4.2.4 TypeScript ⭐新增

**选择理由**：
1. **类型安全**：编译时错误检测
2. **IDE 支持**：优秀的智能提示
3. **可维护性**：大型项目易于维护
4. **现代语法**：支持最新 ES 特性

**用于**：Web UI 前端开发

#### 4.2.5 Crow ⭐新增

**选择理由**：
1. **C++ 原生**：与验证工具无缝集成
2. **轻量级**：简单易用，无复杂依赖
3. **RESTful**：支持路由、JSON 处理
4. **内置 WebSocket**：支持实时通信
5. **高性能**：基于 ASIO，异步处理

**用于**：Web 后端服务器

**示例**：
```cpp
crow::SimpleApp app;

CROW_ROUTE(app, "/api/tasks").methods("GET"_method)
([](const crow::request& req) {
    std::vector<Task> tasks = taskManager.getAllTasks();
    return crow::json::wvalue{{"tasks", tasks}};
});

CROW_ROUTE(app, "/api/tasks").methods("POST"_method)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    Task task = taskManager.createTask(data);
    return crow::json::wvalue{{"taskId", task.id}};
});

app.port(8080).multithreaded().run();
```

#### 4.2.6 WebSocket++ ⭐新增

**选择理由**：
1. **实时通信**：支持任务进度推送
2. **双向通信**：服务器主动推送消息
3. **轻量级**：Header-only 库
4. **与 Crow 集成**：Crow 内置 WebSocket 支持

**用于**：实时进度更新、日志流

#### 4.2.7 nlohmann/json ⭐新增

**选择理由**：
1. **C++ 原生**：现代 C++ JSON 库
2. **易用性**：直观的 API
3. **Header-only**：易于集成
4. **广泛使用**：业界标准

**用于**：JSON 序列化/反序列化

**示例**：
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 序列化
json j;
j["name"] = "CVerifier";
j["version"] = "1.0.0";
std::string s = j.dump();

// 反序列化
json j2 = json::parse(s);
std::string name = j2["name"];
```

### 4.3 依赖库

#### 4.3.1 Z3 4.12+

**选择理由**：
1. **性能优秀**：业界最快的 SMT 求解器之一
2. **功能完整**：支持所有需要的理论
3. **C++ API**：易于集成
4. **增量求解**：支持 push/pop
5. **模型提取**：提供反例

**替代方案**：
- **CVC5**：性能相近，可作为备选
- **Boolector**：专注于位向量，性能可能更好

#### 4.3.2 其他依赖

**最小化原则**：
- 只依赖必要的库
- 避免复杂的依赖链
- 优先使用成熟稳定的库

### 4.4 选型理由

#### 4.4.1 为什么不用其他语言？

**Python**：
- ✅ 优点：开发快速，易用
- ❌ 缺点：性能不足，不适合计算密集型任务

**Java**：
- ✅ 优点：生态成熟
- ❌ 缺点：启动慢，内存占用高

**Rust**：
- ✅ 优点：内存安全，性能接近 C++
- ❌ 缺点：学习曲线陡峭，LLVM 集成不如 C++ 方便

**结论**：C++ 是最佳选择

---

## 5. 架构决策记录

### 5.1 为什么选择 LLVM/Clang？

#### 决策背景

需要解析 C 代码并生成中间表示

#### 考虑的选项

1. **LLVM/Clang**
2. **GCC（libgcc）**
3. **自己编写解析器**

#### 决策：选择 LLVM/Clang

**理由**：

1. **完整的 C 支持**
   - 支持 C11/C17 标准
   - 支持 GCC 扩展
   - 准确的语义分析

2. **LibTooling**
   - 强大的 AST 遍历
   - 易于编写转换工具
   - 丰富的示例

3. **LLVM IR**
   - SSA 形式
   - 优化完善
   - 易于分析

4. **活跃社区**
   - 持续更新
   - 问题修复及时
   - 文档完善

**权衡**：
- ⚠️ 库体积大（~100MB）
- ⚠️ API 复杂，学习曲线陡

**缓解措施**：
- 封装 API，提供简洁接口
- 使用自定义 LLIR 简化后续分析

### 5.2 为什么选择 Z3？

#### 决策背景

需要 SMT 求解器来判断路径的可满足性

#### 考虑的选项

1. **Z3**
2. **CVC5**
3. **Boolector**
4. **MathSAT5（商业）**

#### 决策：选择 Z3

**理由**：

1. **性能**
   - 业界最快之一
   - 增量求解性能好

2. **功能完整**
   - 支持所有需要的理论
   - 支持量化和数组
   - 支持位向量

3. **易用性**
   - C++ API 设计良好
   - 文档完善
   - 示例丰富

4. **开源免费**
   - MIT 许可证
   - 无商业限制

**权衡**：
- ⚠️ 某些特定约束下不如 CVC5

**缓解措施**：
- 设计通用接口 `ISMTSolver`
- 支持切换到 CVC5

### 5.3 为什么使用自定义 LLIR？

#### 决策背景

需要适合符号执行的中间表示

#### 考虑的选项

1. **直接使用 LLVM IR**
2. **自定义 LLIR**
3. **使用其他 IR（如 CPAchecker 的）**

#### 决策：使用自定义 LLIR

**理由**：

1. **轻量级**
   - 只包含必要的指令
   - 易于理解和维护

2. **符号执行友好**
   - 语义清晰
   - 易于建模
   - 支持 Assert 指令

3. **保留源代码信息**
   - 行号和列号
   - 变量名
   - 类型信息

4. **易于扩展**
   - 添加新指令
   - 添加元数据

**权衡**：
- ⚠️ 需要维护 IR 转换层
- ⚠️ 增加复杂度

**缓解措施**：
- LLVM IR → LLIR 转换相对简单
- LLIR 设计尽量简洁

### 5.4 为什么采用混合分析策略？

#### 决策背景

需要平衡准确性和性能

#### 考虑的选项

1. **纯符号执行**
2. **纯抽象解释**
3. **混合分析**

#### 决策：采用混合分析

**理由**：

1. **互补优势**
   - 符号执行：精确，但路径爆炸
   - 抽象解释：快速，但不精确
   - 混合：取长补短

2. **剪枝策略**
   - 抽象解释快速剪枝不可行路径
   - 符号执行专注于可行路径

3. **渐进式**
   - MVP 阶段可以只用符号执行
   - 后续逐步加入抽象解释

**实现策略**：
```
对于每个状态：
  1. 使用抽象解释快速检查
  2. 如果抽象域表明不可行，剪枝
  3. 如果可行，使用符号执行精确分析
```

### 5.5 为什么实现规约分离？ ⭐新增

#### 决策背景

需要支持用户自定义验证规则，同时保持工具的通用性和可扩展性

#### 考虑的选项

1. **硬编码检查规则**：规则内置在工具中
2. **配置文件**：使用 YAML/JSON 配置
3. **嵌入式 DSL**：自定义领域特定语言
4. **规约分离（ACSL风格）**：C语言注解 + 独立编译

#### 决策：规约分离（ACSL风格）

**理由**：

1. **代码与规约分离**
   - 用户代码不包含验证逻辑
   - 规约可以独立更新
   - 验证失败不会影响业务代码

2. **C语言兼容性**
   - 开发者熟悉C语言
   - 无需学习新语言
   - 可以使用C的表达能力

3. **工业验证**
   - ACSL是业界标准（Frama-C）
   - 大量现有规约可复用
   - 工具链成熟

4. **灵活性和可扩展性**
   - 支持自定义验证函数
   - 可以创建复杂的不变量
   - 支持合约式编程

**权衡**：
- ⚠️ 增加解析复杂度
- ⚠️ 需要维护规约库

**缓解措施**：
- 提供丰富的预定义验证函数库
- 提供规约模板和示例
- 支持规约的自动检查

### 5.6 为什么规约使用C语言？ ⭐新增

#### 决策背景

需要选择规约代码的实现语言

#### 考虑的选项

1. **C++**：与工具主体一致
2. **Python**：易用性强
3. **自定义DSL**：专门为规约设计
4. **C语言**：与被验证代码一致

#### 决策：C语言

**理由**：

1. **与被验证代码一致**
   - 被验证代码是C语言
   - 规约代码使用相同语言，易于理解
   - 可以共享类型和宏定义

2. **简单明确**
   - 语法简单，无隐藏行为
   - 容易进行符号执行
   - 编译为机器码后性能高

3. **可移植性**
   - C编译器无处不在
   - 跨平台支持好
   - 易于集成到CI/CD

4. **工业标准**
   - ACSL基于C语言
   - MISRA C等规范使用C
   - 许多安全关键系统使用C

**接口设计**：
```c
// verification.h
#ifdef __cplusplus
extern "C" {
#endif

// 内存验证函数
bool verifiable_read(void* ptr, int size);

// 数值验证函数
bool in_range(int value, int min, int max);

// 断言函数
void assert_true(bool condition, const char* msg);

#ifdef __cplusplus
}
#endif
```

### 5.7 为什么添加Web UI？ ⭐新增

#### 决策背景

需要提供更友好的用户界面和更好的协作体验

#### 考虑的选项

1. **仅CLI工具**：保持简单
2. **桌面GUI**：使用Qt/GTK
3. **IDE插件**：VS Code、JetBrains插件
4. **Web UI**：基于浏览器的界面

#### 决策：Web UI + CLI

**理由**：

1. **跨平台**
   - 浏览器无处不在
   - 无需安装客户端
   - 自动更新

2. **协作功能**
   - 多用户同时访问
   - 任务共享和权限管理
   - 审计日志

3. **任务管理**
   - 可视化任务队列
   - 实时进度监控
   - 批量任务处理

4. **结果可视化**
   - 代码编辑器集成（Monaco）
   - 交互式漏洞查看
   - 报告导出（PDF/SARIF）

5. **远程分析**
   - 云端执行
   - 分布式分析
   - 资源共享

**权衡**：
- ⚠️ 增加开发和维护成本
- ⚠️ 需要额外的服务器

**缓解措施**：
- Web UI作为可选功能
- CLI仍然是核心
- 提供Docker镜像简化部署

### 5.8 为什么选择React而不是Vue/Angular？ ⭐新增

#### 决策背景

需要选择Web前端框架

#### 考虑的选项

1. **React**：Facebook开发的UI库
2. **Vue.js**：渐进式框架
3. **Angular**：完整的平台
4. **Svelte**：编译型框架

#### 决策：React + TypeScript

**理由**：

1. **生态系统成熟**
   - 丰富的组件库（Ant Design, Material-UI）
   - 大量的第三方库
   - 活跃的社区

2. **Monaco Editor集成**
   - VS Code同款编辑器
   - TypeScript原生支持
   - 强大的代码高亮和导航

3. **TypeScript支持**
   - 类型安全
   - IDE智能提示
   - 大型项目可维护

4. **人才储备**
   - React开发者众多
   - 学习资源丰富
   - 招聘容易

5. **灵活性**
   - 库而非框架
   - 可以选择状态管理（Redux/Zustand）
   - 可以选择路由方案

**技术栈**：
```json
{
  "dependencies": {
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "antd": "^5.0.0",
    "axios": "^1.4.0",
    "@monaco-editor/react": "^4.5.0"
  },
  "devDependencies": {
    "typescript": "^5.0.0",
    "vite": "^4.3.0",
    "@types/react": "^18.2.0"
  }
}
```

### 5.9 为什么选择Crow框架？ ⭐新增

#### 决策背景

需要选择C++ Web框架来实现REST API和WebSocket服务

#### 考虑的选项

1. **Crow**：轻量级C++ Web框架
2. **Drogon**：高性能C++14/17 Web框架
3. **Pistache**：HTTP/REST框架
4. **Oat++**：现代C++ Web框架
5. **cpp-httplib**：Header-only库

#### 决策：Crow

**理由**：

1. **C++原生集成**
   - 与验证工具（C++17）无缝集成
   - 可以直接调用核心API
   - 无需额外的语言绑定

2. **轻量级**
   - Header-only（大部分）
   - 依赖少（只需Boost）
   - 编译快速

3. **易用性**
   - 路由定义直观
   - JSON处理简单
   - 内置WebSocket支持

4. **性能**
   - 基于Boost.Asio
   - 异步I/O
   - 多线程支持

5. **与工具集成简单**
   ```cpp
   // 示例：直接调用AnalysisManager
   CROW_ROUTE(app, "/api/analyze").methods("POST"_method)
   [&analyzer](const crow::request& req) {
       auto data = crow::json::load(req.body);
       auto result = analyzer.analyze(data["files"]);
       return crow::json::wvalue{{"result", result}};
   };
   ```

**权衡**：
- ⚠️ 社区相对较小
- ⚠️ 文档不如Drogon完整

**缓解措施**：
- 框架代码简单，易于阅读和调试
- 主要功能稳定
- 如需迁移，API抽象层支持切换

---

## 6. 部署架构

### 6.1 单机部署

**部署模式**：本地工具

**安装方式**：
1. 从源码编译
2. 包管理器（APT、Homebrew、vcpkg）
3. 预编译二进制

**目录结构**：
```
/usr/local/bin/cverifier        # 可执行文件
/usr/local/lib/libcverifier.so  # 库文件
/usr/local/include/cverifier/    # 头文件
/usr/local/etc/cverifier/        # 配置文件
```

### 6.2 容器部署

**Docker 镜像**：

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    clang-15 \
    llvm-15-dev \
    libz3-dev \
    cmake

COPY . /app
WORKDIR /app/build
RUN cmake .. && make install

ENTRYPOINT ["cverifier"]
```

**使用方式**：
```bash
docker run -v $(pwd):/code cverifier /code/example.c
```

### 6.3 云端部署（可选）

**架构**：

```
┌─────────────┐
│  Web UI     │
└──────┬──────┘
       │ HTTP
┌──────▼──────┐
│  API Server │
└──────┬──────┘
       │
┌──────▼──────┐
│  Worker     │  (多实例)
│  - Parser   │
│  - Analyzer │
└─────────────┘
```

**组件**：
- **Web UI**：React/Vue 前端
- **API Server**：REST API，使用 FastAPI/Flask
- **Worker**：分析引擎，队列处理
- **Database**：PostgreSQL 存储分析结果
- **Message Queue**：RabbitMQ/Redis 任务队列

---

## 7. 质量属性

### 7.1 性能优化策略

#### 7.1.1 并行分析

**策略**：多线程并行分析文件和函数

**实现**：
```cpp
class ParallelAnalyzer {
    ThreadPool pool;

    void analyze(std::vector<LLIRFunction*> functions) {
        for (auto* func : functions) {
            pool.enqueue([this, func]() {
                this->analyzeFunction(func);
            });
        }
    }
};
```

**预期效果**：4 线程 → 2.5x 加速

#### 7.1.2 状态合并

**策略**：合并相似的符号状态

**实现**：
```cpp
class StateMerger {
    bool areSimilar(SymbolicState* a, SymbolicState* b) {
        // 比较约束的相似度
        return similarity(a->getConstraints(),
                        b->getConstraints()) > 0.8;
    }

    SymbolicState* merge(SymbolicState* a, SymbolicState* b) {
        // 合并状态，使用闭包
    }
};
```

**预期效果**：减少 50% 状态数

#### 7.1.3 约束缓存

**策略**：缓存约束求解结果

**实现**：
```cpp
class ConstraintCache {
    std::unordered_map<hash_t, SolverResult> cache;

    SolverResult check(const std::vector<Expr*>& constraints) {
        auto hash = computeHash(constraints);
        if (cache.find(hash) != cache.end()) {
            return cache[hash];
        }
        auto result = solver->check(constraints);
        cache[hash] = result;
        return result;
    }
};
```

**预期效果**：减少 30% 求解时间

### 7.2 可扩展性设计

#### 7.2.1 插件式检测器

**接口**：
```cpp
class IVulnerabilityChecker {
public:
    virtual std::vector<VulnerabilityReport> check(
        LLIRFunction* function
    ) = 0;
    virtual std::string getName() const = 0;
};
```

**注册机制**：
```cpp
class CheckerRegistry {
    static std::unordered_map<std::string,
                std::unique_ptr<IVulnerabilityChecker>> registry;

public:
    static void registerChecker(
        const std::string& name,
        std::unique_ptr<IVulnerabilityChecker> checker
    );
};
```

#### 7.2.2 抽象域扩展

**接口**：
```cpp
template<typename T>
class AbstractDomain {
    virtual T bottom() = 0;
    virtual T top() = 0;
    virtual T join(T a, T b) = 0;
    virtual T widen(T a, T b) = 0;
    virtual T transfer(T state, LLIRInstruction* inst) = 0;
};
```

**扩展方式**：
```cpp
class MyCustomDomain : public AbstractDomain<MyState> {
    // 实现虚函数
};
```

### 7.3 容错设计

#### 7.3.1 超时机制

**实现**：
```cpp
class TimeoutWatcher {
    std::atomic<bool> timeout;

    void watch(int seconds) {
        std::thread([this, seconds]() {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            timeout = true;
        }).detach();
    }

    bool hasTimeout() { return timeout; }
};
```

#### 7.3.2 资源限制

**实现**：
```cpp
class ResourceLimiter {
    size_t maxMemory;
    std::atomic<size_t> currentMemory;

    bool checkMemoryLimit() {
        return currentMemory < maxMemory;
    }
};
```

---

## 8. 安全架构

### 8.1 静态分析保证

**原则**：工具不执行用户代码

**保证**：
- 只进行静态分析
- 不动态加载用户代码
- 不使用 JIT 编译

### 8.2 数据隐私保护

**措施**：
1. **本地分析**：所有分析在本地进行
2. **不上传代码**：不发送源代码到远程服务器
3. **清理临时文件**：分析后清理临时文件

### 8.3 输入验证

**措施**：
1. **文件大小限制**：限制单个文件大小（如 10MB）
2. **路径验证**：防止路径遍历攻击
3. **配置验证**：验证配置文件的合法性

---

**文档结束**
