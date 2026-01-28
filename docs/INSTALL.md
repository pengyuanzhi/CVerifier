# CVerifier 快速安装指南

本指南将帮助您在 10 分钟内完成 CVerifier 的安装和配置。

## 📋 系统要求

- **操作系统**: Linux (Ubuntu 20.04+), macOS, 或 Windows 10/11
- **CMake**: 3.20+
- **编译器**: GCC 9+, Clang 10+, 或 MSVC 2019+
- **Python**: 3.8+ (可选，用于脚本)

---

## 🚀 快速安装 (Ubuntu/Debian)

### 步骤 1: 安装依赖

```bash
# 安装 LLVM/Clang
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 15
sudo apt install llvm-15-dev clang-15 libclang-15-dev

# 安装 Z3
wget https://github.com/Z3Prover/z3/releases/download/z3-4.12.4/z3-4.12.4-x64-glibc-2.35.zip
unzip z3-4.12.4-x64-glibc-2.35.zip
sudo cp -r z3-4.12.4.x64 /usr/local/z3

# 安装 CMake
sudo apt install cmake

# 设置环境变量
echo 'export LLVM_DIR=/usr/lib/llvm-15' >> ~/.bashrc
echo 'export Z3_DIR=/usr/local/z3' >> ~/.bashrc
echo 'export PATH=$LLVM_DIR/bin:$Z3_DIR/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### 步骤 2: 编译 CVerifier

```bash
# 克隆仓库
git clone https://github.com/your-org/cverifier.git
cd cverifier

# 运行构建脚本
chmod +x scripts/build.sh
./scripts/build.sh
```

### 步骤 3: 安装（可选）

```bash
cd build
sudo cmake --install .
```

### 步骤 4: 验证安装

```bash
cverifier --version
cverifier --help
```

---

## 🍎 快速安装 (macOS)

```bash
# 安装 Homebrew（如果没有）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install llvm@15 z3 cmake

# 设置环境变量
echo 'export LLVM_DIR=/usr/local/opt/llvm@15' >> ~/.zshrc
echo 'export Z3_DIR=/usr/local/opt/z3' >> ~/.zshrc
source ~/.zshrc

# 克隆并编译
git clone https://github.com/your-org/cverifier.git
cd cverifier
./scripts/build.sh
```

---

## 🪟 快速安装 (Windows)

### 使用 vcpkg（推荐）

```cmd
# 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖
.\vcpkg install llvm:x64-windows z3:x64-windows cmake

# 设置环境变量
setx LLVM_DIR "C:\path\to\vcpkg\installed\x64-windows\share\llvm"
setx Z3_DIR "C:\path\to\vcpkg\installed\x64-windows\share\z3"

# 使用 CMake 编译
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### 或使用预编译版本

1. 下载 LLVM 15+: https://github.com/llvm/llvm-project/releases
2. 下载 Z3 4.12+: https://github.com/Z3Prover/z3/releases
3. 设置环境变量 `LLVM_DIR` 和 `Z3_DIR`
4. 使用 Visual Studio 或 CMake 编译

---

## ✅ 验证安装

运行测试以验证安装是否成功：

```bash
cd build
ctest --output-on-failure
```

---

## 🔧 常见问题

### Q1: CMake 找不到 LLVM

**解决方案**:
```bash
export LLVM_DIR=/usr/lib/llvm-15
cmake ..
```

### Q2: CMake 找不到 Z3

**解决方案**:
```bash
export Z3_DIR=/usr/local/z3
cmake ..
```

### Q3: 编译错误 - 头文件找不到

**解决方案**: 确保 LLVM 和 Z3 的 include 目录在 CPATH 中：
```bash
export CPATH=/usr/local/z3/include:$CPATH
```

### Q4: 链接错误 - 找不到 LLVM 库

**解决方案**: 确保 LLVM 库在 LD_LIBRARY_PATH 中：
```bash
export LD_LIBRARY_PATH=/usr/lib/llvm-15/lib:$LD_LIBRARY_PATH
```

---

## 📚 下一步

- 阅读 [用户手册](user-guide.md) 了解如何使用 CVerifier
- 查看 [快速入门示例](../examples/quick_start.c)
- 阅读 [开发者指南](developer-guide.md) 了解如何贡献代码

---

## 💡 获取帮助

如果遇到问题：
1. 查看 [GitHub Issues](https://github.com/your-org/cverifier/issues)
2. 发送邮件至: your-email@example.com
3. 加入讨论区: [GitHub Discussions](https://github.com/your-org/cverifier/discussions)
