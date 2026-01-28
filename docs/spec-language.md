# CVerifier 规约语言参考

| 文档版本 | 日期 | 作者 | 变更说明 |
|---------|------|------|---------|
| 0.1.0 | 2025-01-28 | CVerifier Team | 初始版本 |

---

## 目录

- [1. 概述](#1-概述)
- [2. ACSL 注解语法](#2-acsl-注解语法)
  - [2.1 基本语法](#21-基本语法)
  - [2.2 前置条件](#22-前置条件)
  - [2.3 后置条件](#23-后置条件)
  - [2.4 循环不变量](#24-循环不变量)
  - [2.5 断言](#25-断言)
- [3. 内置谓词](#3-内置谓词)
  - [3.1 内存谓词](#31-内存谓词)
  - [3.2 指针谓词](#32-指针谓词)
  - [3.3 数值谓词](#33-数值谓词)
- [4. C 语言验证 API](#4-c-语言验证-api)
  - [4.1 内存验证函数](#41-内存验证函数)
  - [4.2 数值验证函数](#42-数值验证函数)
  - [4.3 浮点验证函数](#43-浮点验证函数)
  - [4.4 断言函数](#44-断言函数)
- [5. 示例](#5-示例)
  - [5.1 缓冲区操作](#51-缓冲区操作)
  - [5.2 内存分配](#52-内存分配)
  - [5.3 循环验证](#53-循环验证)
- [6. 最佳实践](#6-最佳实践)

---

## 1. 概述

CVerifier 支持 ACSL（ANSI/ISO C Specification Language）风格的注解来编写验证规约。规约代码使用 C 语言编写，与用户代码分离，独立编译为共享库。

### 1.1 设计原则

1. **代码与规约分离**：规约不污染用户代码
2. **C 语言兼容**：与被验证代码使用相同语言
3. **独立编译**：规约代码编译为 .so/.dll
4. **可组合性**：支持规约复用和组合

### 1.2 文件组织

```
project/
├── src/              # 用户业务代码
│   └── main.c
├── specs/            # 规约代码
│   ├── verification.h
│   ├── buffer.spec.c
│   ├── memory.spec.c
│   └── libverification.so
└── cverifier         # 验证工具
```

---

## 2. ACSL 注解语法

### 2.1 基本语法

ACSL 注解使用特殊的注释语法：

```c
/*@ 注解内容 @*/
```

### 2.2 前置条件 (requires)

用于验证函数调用前的条件：

```c
/*@
  requires \valid_read(ptr);
  requires size > 0;
  requires \separated(ptr, other);
*/
int process(char* ptr, int size, char* other);
```

### 2.3 后置条件 (ensures)

用于验证函数调用后的结果：

```c
/*@
  requires \valid_read(ptr);
  ensures \result >= 0;
  ensures \result <= size;
*/
int safe_copy(char* dest, char* src, int size);
```

### 2.4 循环不变量 (loop invariant)

用于验证循环的正确性：

```c
/*@
  loop invariant 0 <= i <= n;
  loop invariant \valid(arr + (0 .. n-1));
  loop assigns arr[0 .. i-1];
*/
for (int i = 0; i < n; i++) {
    arr[i] = 0;
}
```

### 2.5 断言 (assert)

用于验证代码中的特定条件：

```c
/*@ assert ptr != NULL; */
*ptr = value;
```

---

## 3. 内置谓词

### 3.1 内存谓词

#### `\valid(ptr)` 和 `\valid_read(ptr)`

检查指针指向的内存是否可访问：

```c
/*@
  requires \valid_read(src + (0 .. size-1));
  requires \valid(dest);
*/
void safe_memcpy(char* dest, const char* src, int size);
```

#### `\separated(ptr1, ptr2)`

检查两块内存是否不重叠：

```c
/*@
  requires \valid(dest);
  requires \valid_read(src);
  requires \separated(dest, src);
*/
void safe_copy(char* dest, const char* src);
```

### 3.2 指针谓词

#### `\null(ptr)`

检查指针是否为 NULL：

```c
/*@ ensures \null(\result) || \valid(\result); */
char* allocate_or_null(int size);
```

#### `\initialized(ptr)`

检查指针是否已初始化：

```c
/*@ requires \initialized(ptr); */
void use_value(int* ptr);
```

### 3.3 数值谓词

#### 范围检查

```c
/*@
  requires value >= INT_MIN && value <= INT_MAX;
  requires index >= 0 && index < size;
*/
int array_access(int* array, int size, int index);
```

---

## 4. C 语言验证 API

### 4.1 内存验证函数

#### `verifiable_read(ptr, size)`

验证内存可读性：

```c
bool verifiable_read(void* ptr, int size);

// 示例
if (!verifiable_read(buffer, 100)) {
    return -1;  // 内存不可读
}
```

#### `verifiable_write(ptr, size)`

验证内存可写性：

```c
bool verifiable_write(void* ptr, int size);

// 示例
if (!verifiable_write(buffer, size)) {
    return -1;
}
```

#### `verifiable_separated(ptr1, ptr2, size)`

验证两块内存不重叠：

```c
bool verifiable_separated(void* ptr1, void* ptr2, int size);

// 示例
if (!verifiable_separated(dest, src, n)) {
    return -1;  // 内存重叠
}
```

### 4.2 数值验证函数

#### `in_range(value, min, max)`

验证值在范围内：

```c
bool in_range(int value, int min, int max);

// 示例
if (!in_range(index, 0, size - 1)) {
    return -1;  // 索引越界
}
```

#### `will_add_overflow(a, b)`

检测无符号加法溢出：

```c
bool will_add_overflow(unsigned int a, unsigned int b);

// 示例
if (will_add_overflow(a, b)) {
    return -1;  // 会溢出
}
```

### 4.3 浮点验证函数

#### `is_nan(value)`

检测是否为 NaN：

```c
bool is_nan(double value);

// 示例
double result = x / y;
if (is_nan(result)) {
    return -1;  // 产生 NaN
}
```

#### `will_multiply_overflow(a, b)`

检测浮点乘法溢出：

```c
bool will_multiply_overflow(double a, double b);

// 示例
if (will_multiply_overflow(large, large)) {
    return -1;  // 会溢出到 Inf
}
```

### 4.4 断言函数

#### `assert_true(condition, msg)`

运行时断言：

```c
void assert_true(bool condition, const char* msg);

// 示例
assert_true(ptr != NULL, "Pointer is NULL");
```

#### `VERIFICATION_CHECK(cond)`

验证专用检查（不终止程序）：

```c
#define VERIFICATION_CHECK(cond) \
    verification_check(cond, __FILE__, __LINE__)

// 示例
if (!VERIFICATION_CHECK(size > 0)) {
    return -1;
}
```

---

## 5. 示例

### 5.1 缓冲区操作

```c
/*@
  requires \valid_read(src + (0 .. size-1));
  requires \valid(dest);
  requires size > 0;
  requires \separated(dest, src);
  assigns dest[0 .. size-1];

  ensures \result == 0;
*/
int safe_memcpy(char* dest, const char* src, int size) {
    if (!verifiable_read((void*)src, size)) {
        return -1;
    }
    if (!verifiable_write(dest, size)) {
        return -1;
    }
    if (!verifiable_separated(dest, (void*)src, size)) {
        return -1;
    }

    for (int i = 0; i < size; i++) {
        /*@ assert 0 <= i < size; */
        dest[i] = src[i];
    }

    return 0;
}
```

### 5.2 内存分配

```c
/*@
  requires size > 0;
  ensures \null(\result) || \valid(\result);
  ensures \null(\result) || \valid(\result + (0 .. size-1));
*/
void* safe_malloc(int size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        return NULL;
    }
    return ptr;
}
```

### 5.3 循环验证

```c
/*@
  requires \valid_read(arr + (0 .. n-1));
  requires n >= 0;
  assigns result;

  ensures \result >= 0;
*/
int sum_array(int* arr, int n) {
    int result = 0;

    /*@
      loop invariant 0 <= i <= n;
      loop invariant result == \sum(0, i-1, \lambda integer k; arr[k]);
      loop assigns i, result;
    */
    for (int i = 0; i < n; i++) {
        /*@ assert 0 <= i < n; */
        result += arr[i];

        // 检测整数溢出
        if (will_signed_add_overflow(result, arr[i])) {
            return -1;  // 溢出
        }
    }

    return result;
}
```

---

## 6. 最佳实践

### 6.1 规约编写原则

1. **最小化前置条件**：只要求必要的条件
2. **最大化后置条件**：尽可能详细地描述结果
3. **使用循环不变量**：帮助验证循环
4. **分离关注点**：每个函数只做一件事

### 6.2 性能考虑

1. **避免过度的路径探索**：合理使用前置条件剪枝
2. **使用抽象解释**：快速排除不可行路径
3. **分阶段验证**：先验证关键函数

### 6.3 调试技巧

1. **使用 VERIFICATION_CHECK**：收集更多错误信息
2. **添加断言**：验证中间状态
3. **查看反例**：理解为什么验证失败

---

## 附录 A: 完整 API 列表

详见 `specs/verification.h`

## 附录 B: 编译规约库

```bash
cd specs
mkdir build && cd build
cmake ..
make
```

生成 `libverification.so` 或 `verification.dll`

## 附录 C: 相关文档

- [架构设计文档](architecture.md) - 第 3.6 节：规约层
- [详细设计文档](design.md) - 第 6 节：规约系统设计
- [验证 API 参考](verification-api.md) - 完整 API 文档
- [插件开发指南](plugin-api.md) - 插件开发

---

**文档结束**
