# 构建缓存清理说明

## 问题

如果遇到编译错误指向已经不存在的代码（如 `const_cast` 或 `newFrontendActionFactory`），这是 CMake 构建缓存问题。

## 解决方案

### 方法 1: 清理构建目录（推荐）

```bash
cd build
rm -rf *
cmake ..
make -j4
```

### 方法 2: 完全清理重新构建

```bash
cd D:/AI/homework/ClaudeCode/W-AVC
rm -rf build
mkdir build
cd build
cmake ..
make -j4
```

### Windows 用户

```cmd
cd D:\AI\homework\ClaudeCode\W-AVC\build
rmdir /s /q *
cmake ..
cmake --build . --config Release
```

## 验证

编译成功后，应该看到：
- `cverifier-core` 库编译成功
- `cverifier-analyzer` 库编译成功  
- `cverifier-frontend` 库编译成功（如果启用了 LLVM）
- `test_z3`、`test_abstract`、`test_clang` 测试程序编译成功

## 常见编译错误已修复

所有以下错误都已在代码中修复：
- ✅ `const_cast` 类型错误 - 改用 `runToolOnCode`
- ✅ `newFrontendActionFactory` lambda 错误 - 改用 `runToolOnCode`
- ✅ `DeclarationName::str()` 已弃用 - 改用 `getAsString()`
- ✅ `StringRef::getAsString()` 不存在 - 改用 `str()`
- ✅ `SourceLocation` 命名空间冲突 - 统一使用 `cverifier::SourceLocation`
- ✅ `LLIRInstruction` 抽象类 - 实现 `getValueType()`

如果仍然看到这些错误，请清理构建缓存！
