# CVerifier

<div align="center">

**基于符号执行和抽象解释的C语言形式化验证工具**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![LLVM](https://img.shields.io/badge/LLVM-15+-green.svg)](https://llvm.org/)
[![Z3](https://img.shields.io/badge/Z3-4.12+-orange.svg)](https://github.com/Z3Prover/z3)

</div>

---

## 📋 项目简介

CVerifier 是一个现代化的 C 语言形式化验证工具，采用符号执行和抽象解释技术，能够自动检测 C 代码中的安全漏洞。本项目旨在提供工业级的静态分析能力，同时保持快速的迭代开发。

### ✨ 核心特性

- **🔍 全面的漏洞检测**
  - 缓冲区溢出（栈/堆/静态数组）
  - 空指针解引用
  - 内存泄漏
  - 整数溢出

- **🚀 高性能分析**
  - 符号执行 + 抽象解释混合分析
  - 并行路径探索
  - 智能状态合并和剪枝
  - 增量分析支持

- **🛠️ 易于使用**
  - 简洁的命令行接口
  - 灵活的配置系统
  - 多种输出格式（Console/SARIF/JSON）
  - IDE 集成支持

- **🔧 可扩展架构**
  - 模块化设计
  - 插件式检查器
  - 自定义抽象域支持

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    用户接口层                            │
│              CLI 工具 | IDE 插件 | Web UI               │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   核心分析层                             │
│         符号执行引擎 | 抽象解释器 | 漏洞检测器           │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   中间表示层                             │
│              LLVM IR | 自定义 LLIR                      │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   基础设施层                             │
│              Z3 SMT Solver | 约束求解器                 │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   前端处理层                             │
│              Clang Parser | AST Builder                 │
└─────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 描述 |
|------|------|
| **Frontend** | 基于 Clang 的 C 代码解析器，支持 C11/C17 标准 |
| **LLIR** | 轻量级中间表示，用于符号执行的语义建模 |
| **Symbolic Execution** | 符号执行引擎，支持多种路径探索策略 |
| **Abstract Interpretation** | 抽象解释框架，包含多种抽象域 |
| **Checkers** | 模块化漏洞检测器 |
| **Z3 Solver** | SMT 约束求解器接口 |

---

## 📚 文档

### 核心文档

| 文档 | 描述 |
|------|------|
| [📋 需求规格说明书](docs/requirements.md) | 完整的功能和非功能需求、验收标准 |
| [🏗️ 架构设计文档](docs/architecture.md) | 系统架构、技术选型、架构决策记录 |
| [🔧 详细设计文档](docs/design.md) | 模块设计、接口设计、算法设计 |
| [🗺️ 项目路线图](docs/roadmap.md) | 开发阶段、里程碑、资源规划 |
| [🚀 快速安装指南](docs/INSTALL.md) | 快速安装和配置指南 |

### 快速链接

- **新手入门**：[快速安装指南](docs/INSTALL.md) → [需求规格说明书](docs/requirements.md)
- **架构理解**：[架构设计文档](docs/architecture.md) → [详细设计文档](docs/design.md)
- **开发计划**：[项目路线图](docs/roadmap.md)
- **API 参考**：[公共头文件](include/cverifier/)（开发中）

### 文档结构

```
docs/
├── requirements.md      # 软件需求规格说明书（SRS）
├── architecture.md      # 软件架构设计文档
├── design.md           # 详细设计文档
├── roadmap.md          # 项目路线图
└── INSTALL.md          # 快速安装指南
```

---

## 📦 安装指南

### 系统要求

- **操作系统**: Windows 10/11, Linux (Ubuntu 20.04+), macOS
- **编译器**: GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake**: 3.20+
- **Python**: 3.8+（用于脚本）

### 依赖安装

#### 1. 安装 LLVM 15+

**Ubuntu/Debian:**
```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 15
sudo apt install llvm-15-dev clang-15 libclang-15-dev
```

**macOS:**
```bash
brew install llvm@15
echo 'export LLVM_DIR=/usr/local/opt/llvm@15' >> ~/.zshrc
source ~/.zshrc
```

**Windows (使用 vcpkg):**
```cmd
vcpkg install llvm:x64-windows
set LLVM_DIR=C:\path\to\vcpkg\installed\x64-windows\share\llvm
```

#### 2. 安装 Z3 SMT Solver

**Ubuntu/Debian:**
```bash
wget https://github.com/Z3Prover/z3/releases/download/z3-4.12.4/z3-4.12.4-x64-glibc-2.35.zip
unzip z3-4.12.4-x64-glibc-2.35.zip
sudo cp -r z3-4.12.4.x64 /usr/local/z3
echo 'export Z3_DIR=/usr/local/z3' >> ~/.bashrc
source ~/.bashrc
```

**macOS:**
```bash
brew install z3
```

**Windows (使用 vcpkg):**
```cmd
vcpkg install z3:x64-windows
```

#### 3. 验证安装

```bash
# 检查 LLVM
clang --version    # 应显示 15+
llvm-config --version

# 检查 Z3
z3 --version       # 应显示 4.12+
```

### 编译 CVerifier

```bash
# 克隆仓库
git clone https://github.com/your-org/cverifier.git
cd cverifier

# 创建构建目录
mkdir build && cd build

# 配置（Debug 模式）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 或（Release 模式）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 编译
cmake --build . -j$(nproc)

# 运行测试
ctest --output-on-failure

# 安装（可选）
sudo cmake --install .
```

---

## 🚀 使用指南

### 基本用法

```bash
# 分析单个文件
cverifier test.c

# 分析多个文件
cverifier file1.c file2.c file3.c

# 使用配置文件
cverifier --config=configs/aggressive.yaml src/*.c

# 指定入口函数
cverifier --entry-function=main test.c

# 输出 SARIF 格式报告
cverifier --output-format=sarif --output=report.sarif test.c
```

### 命令行选项

| 选项 | 描述 | 示例 |
|------|------|------|
| `--config=<file>` | 使用配置文件 | `--config=default.yaml` |
| `--enable=<checkers>` | 启用检查器（逗号分隔） | `--enable=buffer-overflow,null-pointer` |
| `--disable=<checkers>` | 禁用检查器 | `--disable=memory-leak` |
| `--enable=all` | 启用所有检查器 | `--enable=all` |
| `--output-format=<fmt>` | 输出格式 | `--output-format=sarif` |
| `--output=<file>` | 输出文件 | `--output=report.sarif` |
| `--timeout=<sec>` | 超时时间（秒） | `--timeout=300` |
| `--max-depth=<n>` | 最大探索深度 | `--max-depth=100` |
| `--entry-function=<name>` | 入口函数 | `--entry-function=main` |
| `--verbose` | 详细输出 | `--verbose` |
| `--version` | 显示版本信息 | `--version` |
| `--help` | 显示帮助 | `--help` |

### 配置文件示例

```yaml
# configs/default.yaml
analysis:
  timeout: 300
  max-depth: 100
  max-states: 10000
  parallel-threads: 4

  path-exploration:
    strategy: hybrid  # dfs, bfs, random, hybrid
    merge-similar-states: true

  abstraction:
    enabled: true
    domain: interval  # constant, interval, octagon

checkers:
  enable:
    - buffer-overflow
    - null-pointer
    - memory-leak
    - integer-overflow

  buffer-overflow:
    check-heap: true
    check-stack: true
    check-static: true

solver:
  backend: z3
  timeout: 10

reporting:
  format: console
  include-trace: true
  include-source-snippet: true
```

---

## 📝 示例

### 检测缓冲区溢出

```c
// examples/buffer_overflow.c
#include <string.h>

void vulnerable_function(char* input) {
    char buffer[10];
    strcpy(buffer, input);  // ⚠️ 缓冲区溢出风险
}

int main(int argc, char** argv) {
    if (argc > 1) {
        vulnerable_function(argv[1]);
    }
    return 0;
}
```

**分析结果:**
```
$ cverifier examples/buffer_overflow.c

[ERROR] Buffer Overflow Detected
  File: examples/buffer_overflow.c, Line: 6
  Severity: CRITICAL
  Checker: buffer-overflow

  Code Snippet:
     5  void vulnerable_function(char* input) {
     6      char buffer[10];
  >>  7      strcpy(buffer, input);
     8  }

  Description:
    Potential buffer overflow: 'input' may be larger than 10 bytes

  Error Trace:
    1. examples/buffer_overflow.c:7
       Input length: 15 bytes
       Buffer size: 10 bytes
       Condition: 15 > 10 is TRUE (overflow!)

Summary:
  Files analyzed: 1
  Functions analyzed: 2
  Total errors found: 1
  - Critical: 1
```

### 检测空指针解引用

```c
// examples/null_pointer.c
#include <stdlib.h>

void process_data(int* ptr) {
    if (ptr == NULL) {
        return;
    }
    *ptr = 42;  // ✅ 安全
}

void vulnerable_function(int* ptr) {
    *ptr = 42;  // ⚠️ 未检查空指针
}

int main() {
    int* p = NULL;
    vulnerable_function(p);
    return 0;
}
```

### 检测内存泄漏

```c
// examples/memory_leak.c
#include <stdlib.h>

void allocate_memory() {
    int* ptr = (int*)malloc(sizeof(int) * 10);
    *ptr = 42;
    // ⚠️ 内存泄漏：未调用 free(ptr)
}

int main() {
    allocate_memory();
    return 0;
}
```

---

## 🧪 测试

### 运行测试

```bash
cd build

# 运行所有测试
ctest

# 运行特定测试
ctest -R unit
ctest -R integration

# 详细输出
ctest --verbose

# 运行性能测试
ctest -R benchmark
```

### 编写测试用例

```cpp
// tests/unit/Checkers/TestBufferOverflowChecker.cpp
#include <gtest/gtest.h>
#include "cverifier/Analyzer.h"

TEST(BufferOverflowChecker, DetectsStaticArrayOverflow) {
    // 创建测试用例
    auto module = createTestModule(R"(
        void test() {
            char buf[10];
            buf[10] = 'a';
        }
    )");

    BufferOverflowChecker checker(std::make_unique<Z3Solver>());
    auto reports = checker.check(module->getFunction("test"));

    EXPECT_EQ(reports.size(), 1);
    EXPECT_EQ(reports[0].type, VulnerabilityType::BufferOverflow);
}
```

---

## 📚 文档

- [架构设计文档](docs/architecture.md)
- [用户手册](docs/user-guide.md)
- [开发者指南](docs/developer-guide.md)
- [API 参考文档](docs/api-reference.md)

---

## 🛣️ 开发路线图

### ✅ 阶段 1: MVP (已实现)
- [x] Clang 集成
- [x] 基础符号执行
- [x] 简单漏洞检测
- [x] Z3 集成

### 🔄 阶段 2: 核心增强 (进行中)
- [ ] 抽象解释框架
- [ ] 混合分析
- [ ] 动态内存分析
- [ ] 性能优化

### 📅 阶段 3: 工业质量 (计划中)
- [ ] 完整 C 标准支持
- [ ] 外部函数建模
- [ ] 并行分析
- [ ] IDE 插件

### 🚀 阶段 4: 高级特性 (规划中)
- [ ] 数据流分析
- [ ] 修复建议
- [ ] Web UI
- [ ] 云端分析

---

## 🤝 贡献指南

我们欢迎所有形式的贡献！

### 如何贡献

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

### 代码规范

- 遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- 添加单元测试
- 更新文档
- 保持代码简洁清晰

---

## 📄 许可证

本项目采用 [MIT 许可证](LICENSE)

---

## 👥 作者与致谢

- 主要开发者：Your Name
- 感谢以下开源项目：
  - [LLVM/Clang](https://llvm.org/)
  - [Z3 Theorem Prover](https://github.com/Z3Prover/z3)
  - [CPAchecker](https://cpachecker.sosy-lab.org/)

---

## 📮 联系方式

- 问题反馈：[GitHub Issues](https://github.com/your-org/cverifier/issues)
- 邮件：your-email@example.com
- 讨论区：[GitHub Discussions](https://github.com/your-org/cverifier/discussions)

---

<div align="center">

**如果觉得项目有帮助，请给个 ⭐ Star 支持一下！**

Made with ❤️ by CVerifier Team

</div>
