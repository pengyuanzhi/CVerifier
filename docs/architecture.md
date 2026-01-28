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
  - [2.1 六层架构设计](#21-六层架构设计)
  - [2.2 架构图](#22-架构图)
  - [2.3 架构说明](#23-架构说明)
  - [2.4 数据流](#24-数据流)
- [3. 核心组件](#3-核心组件)
  - [3.1 前端处理层](#31-前端处理层)
  - [3.2 中间表示层](#32-中间表示层)
  - [3.3 核心分析层](#33-核心分析层)
  - [3.4 基础设施层](#34-基础设施层)
  - [3.5 用户接口层](#35-用户接口层)
- [4. 技术选型](#4-技术选型)
  - [4.1 编程语言](#41-编程语言)
  - [4.2 框架选择](#42-框架选择)
  - [4.3 依赖库](#43-依赖库)
  - [4.4 选型理由](#44-选型理由)
- [5. 架构决策记录](#5-架构决策记录)
  - [5.1 为什么选择 LLVM/Clang？](#51-为什么选择-llvmclang)
  - [5.2 为什么选择 Z3？](#52-为什么选择-z3)
  - [5.3 为什么使用自定义 LLIR？](#53-为什么使用自定义-llir)
  - [5.4 为什么采用混合分析策略？](#54-为什么采用混合分析策略)
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

### 2.1 六层架构设计

CVerifier 采用六层架构，从上到下依次为：

1. **用户接口层**：提供 CLI 工具、IDE 插件、Web UI（可选）
2. **分析配置层**：配置管理、检查规则、报告生成
3. **核心分析层**：符号执行引擎、抽象解释器、混合分析器
4. **中间表示层**：IR 转换、IR 优化、CFG 构建
5. **基础设施层**：SMT 求解器、约束求解器、定理证明器
6. **前端处理层**：预处理器、C Parser、AST 构建

### 2.2 架构图

```
┌─────────────────────────────────────────────────────────┐
│                    用户接口层                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ CLI 工具  │  │ IDE 插件 │  │ Web UI   │              │
│  └──────────┘  └──────────┘  └──────────┘              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   分析配置层                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │配置管理器 │  │ 检查规则 │  │ 报告生成 │              │
│  └──────────┘  └──────────┘  └──────────┘              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   核心分析层                             │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐        │
│  │ 符号执行引擎 │  │ 抽象解释器 │  │ 混合分析器 │        │
│  └────────────┘  └────────────┘  └────────────┘        │
│  ┌────────────────────────────────────────────┐        │
│  │         漏洞检测器                          │        │
│  │  BufferOverflow │ NullPointer │ MemoryLeak │        │
│  └────────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   中间表示层                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ IR 转换   │  │ IR 优化   │  │ CFG 构建  │              │
│  └──────────┘  └──────────┘  └──────────┘              │
│  ┌────────────────────────────────────────────┐        │
│  │         LLIR (轻量级中间表示)               │        │
│  └────────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   基础设施层                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ Z3 求解器 │  │ 约束构建器 │  │ 模型提取器 │              │
│  └──────────┘  └──────────┘  └──────────┘              │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   前端处理层                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ 预处理器  │  │ Clang    │  │ AST 构建 │              │
│  │          │  │ Parser   │  │          │              │
│  └──────────┘  └──────────┘  └──────────┘              │
└─────────────────────────────────────────────────────────┘
```

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
