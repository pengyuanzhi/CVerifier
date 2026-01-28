# CVerifier C API 参考手册

| 文档版本 | 日期 | 作者 | 变更说明 |
|---------|------|------|---------|
| 0.1.0 | 2025-01-28 | CVerifier Team | 初始版本 |

---

## 目录

- [1. 概述](#1-概述)
- [2. 内存验证函数](#2-内存验证函数)
- [3. 数值验证函数](#3-数值验证函数)
- [4. 浮点验证函数](#4-浮点验证函数)
- [5. 断言函数](#5-断言函数)
- [6. ACSL 谓词宏](#6-acsl-谓词宏)
- [7. 示例](#7-示例)

---

## 1. 概述

本文档描述 CVerifier 提供的 C 语言验证 API。这些函数用于在规约代码中编写验证逻辑。

**头文件**：`specs/verification.h`

**编译**：`gcc -shared -fPIC -o libverification.so specs/*.c`

---

## 2. 内存验证函数

### 2.1 `verifiable_read`

**函数原型**：
```c
bool verifiable_read(void* ptr, int size);
```

**功能**：验证内存可读性

**参数**：
- `ptr`：内存指针
- `size`：字节数

**返回值**：
- `true`：内存可读
- `false`：内存不可读（NULL 或越界）

**示例**：
```c
if (!verifiable_read(buffer, 100)) {
    return -1;
}
```

### 2.2 `verifiable_write`

**函数原型**：
```c
bool verifiable_write(void* ptr, int size);
```

**功能**：验证内存可写性

**参数**：
- `ptr`：内存指针
- `size`：字节数

**返回值**：
- `true`：内存可写
- `false`：内存不可写

### 2.3 `verifiable_pointer`

**函数原型**：
```c
bool verifiable_pointer(void* ptr);
```

**功能**：验证指针有效性（非 NULL）

**参数**：
- `ptr`：指针

**返回值**：
- `true`：指针有效
- `false`：指针为 NULL

### 2.4 `verifiable_separated`

**函数原型**：
```c
bool verifiable_separated(void* ptr1, void* ptr2, int size);
```

**功能**：验证两块内存不重叠

**参数**：
- `ptr1`：第一块内存
- `ptr2`：第二块内存
- `size`：内存大小

**返回值**：
- `true`：内存不重叠
- `false`：内存重叠

---

## 3. 数值验证函数

### 3.1 `in_range`

**函数原型**：
```c
bool in_range(int value, int min, int max);
```

**功能**：检查值在范围内

**参数**：
- `value`：待验证的值
- `min`：最小值（包含）
- `max`：最大值（包含）

**返回值**：
- `true`：在范围内
- `false`：超出范围

### 3.2 `is_positive`

**函数原型**：
```c
bool is_positive(int value);
```

**功能**：检查无符号整数为正数

### 3.3 `is_nonzero`

**函数原型**：
```c
bool is_nonzero(int value);
```

**功能**：检查整数为非零

### 3.4 `will_add_overflow`

**函数原型**：
```c
bool will_add_overflow(unsigned int a, unsigned int b);
```

**功能**：检测无符号加法溢出

### 3.5 `will_signed_add_overflow`

**函数原型**：
```c
bool will_signed_add_overflow(int a, int b);
```

**功能**：检测有符号加法溢出

---

## 4. 浮点验证函数

### 4.1 `is_nan`

**函数原型**：
```c
bool is_nan(double value);
```

**功能**：检测浮点数是否为 NaN

**实现**：
```c
bool is_nan(double value) {
    return value != value;  // IEEE 754: NaN != NaN
}
```

### 4.2 `is_infinity`

**函数原型**：
```c
bool is_infinity(double value);
```

**功能**：检测浮点数是否为无穷大

### 4.3 `is_finite`

**函数原型**：
```c
bool is_finite(double value);
```

**功能**：检测浮点数是否为有限数（非 NaN，非 Inf）

### 4.4 `will_multiply_overflow`

**函数原型**：
```c
bool will_multiply_overflow(double a, double b);
```

**功能**：检测浮点乘法是否会上溢

### 4.5 `will_lose_precision`

**函数原型**：
```c
bool will_lose_precision(double large, double small);
```

**功能**：检测浮点运算精度损失

---

## 5. 断言函数

### 5.1 `assert_true`

**函数原型**：
```c
void assert_true(bool condition, const char* msg);
```

**功能**：运行时断言

**注意**：此函数不会终止程序，只记录错误

### 5.2 `STATIC_ASSERT`

**宏定义**：
```c
#define STATIC_ASSERT(cond) _Static_assert(cond, #cond)
```

**功能**：编译时断言

**示例**：
```c
STATIC_ASSERT(sizeof(int) == 4);
```

### 5.3 `VERIFICATION_CHECK`

**宏定义**：
```c
#define VERIFICATION_CHECK(cond) \
    verification_check(cond, __FILE__, __LINE__)
```

**功能**：运行时检查（规约验证专用）

**返回值**：
- `true`：条件成立
- `false`：条件不成立（不终止程序）

---

## 6. ACSL 谓词宏

### 6.1 `valid(ptr)`

检查指针指向的内存区域可读

```c
#define valid(ptr) verifiable_read(ptr, sizeof(*(ptr)))
```

### 6.2 `valid_read(ptr)`

同 `valid(ptr)`

### 6.3 `separated(ptr1, ptr2)`

检查两个指针指向的内存不重叠

```c
#define separated(ptr1, ptr2) \
    verifiable_separated(ptr1, ptr2, sizeof(*(ptr1)))
```

### 6.4 `null(ptr)`

检查指针是否为 NULL

```c
#define null(ptr) ((ptr) == NULL)
```

### 6.5 `initialized(ptr)`

检查指针是否已初始化（非 NULL）

```c
#define initialized(ptr) ((ptr) != NULL)
```

---

## 7. 示例

### 7.1 安全的字符串复制

```c
/*@
  requires \valid_read(src + (0 .. size-1));
  requires \valid(dest);
  requires \separated(dest, src);
  assigns dest[0 .. size-1];

  ensures \result == 0;
*/
int safe_strcpy(char* dest, const char* src, int size) {
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
        if (src[i] == '\0') {
            dest[i] = '\0';
            return 0;
        }
        dest[i] = src[i];
    }

    dest[size - 1] = '\0';
    return 0;
}
```

### 7.2 安全的数组访问

```c
/*@
  requires \valid_read(array + (0 .. size-1));
  requires index >= 0 && index < size;

  ensures \result >= 0;
*/
int safe_array_access(int* array, int size, int index) {
    VERIFICATION_CHECK(in_range(index, 0, size - 1));
    VERIFICATION_CHECK(verifiable_read(array, size));

    return array[index];
}
```

### 7.3 浮点运算验证

```c
/*@
  requires is_finite(a);
  requires is_finite(b);
  requires !will_multiply_overflow(a, b);

  ensures is_finite(\result);
*/
double safe_multiply(double a, double b) {
    if (is_nan(a) || is_nan(b)) {
        return NAN;  // 传播 NaN
    }

    if (is_infinity(a) || is_infinity(b)) {
        return INFINITY;  // 简化处理
    }

    if (will_multiply_overflow(a, b)) {
        return INFINITY;
    }

    return a * b;
}
```

---

## 附录 A: 完整头文件

详见 `specs/verification.h`

## 附录 B: 链接库

```cmake
target_link_libraries(your_program PRIVATE
    verification
)
```

## 附录 C: 相关文档

- [规约语言参考](spec-language.md)
- [规约代码示例](../specs/buffer.spec.c)
- [架构设计文档](architecture.md#36-规约层)

---

**文档结束**
