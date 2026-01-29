# Clang 前端集成说明

## 概述

CVerifier 现在使用 **libclang C API** 来实现 C 代码解析功能。这是一个简化的方案，可以与预编译的 LLVM 安装配合使用。

## 实现方案

### 为什么选择 libclang C API？

1. **更简单的链接** - 只需要链接 `libclang` 一个库
2. **跨平台兼容** - 适用于 Windows、Linux、macOS
3. **稳定接口** - C API 比 C++ API 更稳定
4. **易于部署** - 大多数 LLVM 发行版都包含 libclang

### 完整 Clang C++ API 的问题

完整的 Clang C++ API（如 `clangAST`, `clangBasic` 等）：
- ✅ 功能更强大
- ✅ 提供更细粒度的控制
- ❌ 需要链接数十个库
- ❌ 版本兼容性差
- ❌ 通常需要从源码编译 LLVM

## 当前实现

### LibClangParser 类

位置：`src/frontend/libclang/LibClangParser.cpp`

**当前功能**：
- ✅ 解析 C 源文件
- ✅ 提取函数声明
- ✅ 创建 LLIR 模块结构
- ✅ 基本的 AST 遍历

**已知限制**：
- ⚠️ 函数体转换未完全实现
- ⚠️ 表达式转换简化
- ⚠️ 控制流表示基础

## 编译和配置

### 自动配置（推荐）

CMake 会自动查找 libclang：

```bash
cd build
cmake ..
make
```

CMake 会在以下位置查找 libclang：
- `/usr/lib` 和 `/usr/local/lib` (Linux/macOS)
- `C:/Program Files/LLVM/lib` (Windows)
- `${LLVM_LIBRARY_DIR}` (如果设置了 LLVM_DIR)

### 手动配置

如果自动查找失败，可以手动指定：

```bash
# Linux/macOS
cmake .. -DLIBCLANG_LIB_PATH=/usr/lib/llvm-15/lib/libclang.so

# Windows
cmake .. -DLIBCLANG_LIB_PATH="C:/Program Files/LLVM/lib/libclang.lib"
```

或者设置 LLVM_DIR：

```bash
cmake .. -DLLVM_DIR="C:/Program Files/LLVM/lib/cmake/llvm"
```

## 使用方法

### 解析 C 文件

```bash
# 基本用法
./cverifier test.c

# 详细输出
./cverifier --verbose test.c

# 查看帮助
./cverifier --help
```

### 示例

测试文件：`tests/integration/buffer-overflow/test_buffer_overflow.c`

```bash
cd build
./cverifier ../tests/integration/buffer-overflow/test_buffer_overflow.c
```

## 测试

当前 libclang 前端可以：

1. **解析简单函数**
```c
int add(int a, int b) {
    return a + b;
}
```

2. **处理控制流**
```c
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
```

3. **识别函数声明**
- 提取函数名
- 提取参数列表
- 创建对应的 LLIR 结构

## 未来改进方向

### 短期（当前版本）

1. ✅ **实现基本的 C 解析** - 已完成
2. 🔄 **完善函数体转换** - 进行中
3. ⏳ **添加表达式转换**
4. ⏳ **改进 CFG 生成**

### 中期（下一版本）

1. 完整的语句转换（if/while/for/switch）
2. 复杂表达式支持
3. 结构体和联合体支持
4. 指针算术支持

### 长期（未来版本）

1. 如果需要完整功能，考虑从源码编译 LLVM/Clang
2. 或者使用更高级的库（如 LibTooling）

### 完整 Clang C++ API 方案

如果您需要完整的 Clang C++ API 功能：

#### 选项 1：从源码编译 LLVM/Clang

```bash
# 获取 LLVM 源码
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout llvmorg-15.x.x

# 编译（需要 1-2 小时）
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
  -DCMAKE_INSTALL_PREFIX=/usr/local ../llvm
ninja
ninja install
```

#### 选项 2：使用包管理器

**Ubuntu/Debian**:
```bash
sudo apt install clang-15 clang-tools-15 libclang-15-dev
```

**macOS (Homebrew)**:
```bash
brew install llvm@15
```

#### 选项 3：使用预构建的二进制包

某些发行版提供完整的开发包：
- Ubuntu: `llvm-15-dev`
- Fedora: `llvm15-devel`
- Arch: `llvm`

## 故障排除

### 问题：找不到 libclang

**症状**：
```
Clang libraries linked: LIBCLANG_LIB-NOTFOUND
```

**解决方案**：
1. 确认 LLVM 已正确安装
2. 设置 LLVM_DIR 环境变量
3. 手动指定库路径

### 问题：头文件找不到

**症状**：
```
fatal error: clang-c/Index.h: No such file or directory
```

**解决方案**：
```bash
# Ubuntu/Debian
sudo apt install libclang-dev

# macOS
brew install llvm@15

# 或手动指定包含路径
cmake .. -DLIBCLANG_INCLUDE_DIR=/path/to/llvm/include
```

### 问题：链接错误

**症状**：
```
undefined reference to 'clang_createIndex'
```

**解决方案**：
确保链接了 libclang 库，检查 CMake 输出中的：
```
Found libclang: /path/to/libclang
```

## 参考资源

### libclang API 文档

- [libclang API](https://clang.llvm.org/doxygen/group__CINDEX.html)
- [libclang 教程](https://github.com/llvm/llvm-project/tree/main/clang/tools/libclang)

### LLVM/Clang 文档

- [LLVM Getting Started](https://llvm.org/docs/GettingStarted.html)
- [Clang Tools Extra](https://clang.llvm.org/docs/ClangTools.html)

## 总结

当前的 libclang 方案是一个**实用的折衷**：

**优点**：
- ✅ 可以使用预编译的 LLVM
- ✅ 链接配置简单
- ✅ 跨平台支持良好
- ✅ 基本功能可用

**缺点**：
- ⚠️ 功能相对有限
- ⚠️ 某些高级特性需要额外工作

对于 CVerifier 的当前阶段，这个方案足够支持：
- 基本的 C 代码解析
- 函数级别分析
- 漏洞检测演示

如果未来需要更强大的功能，可以切换到完整的 Clang C++ API。
