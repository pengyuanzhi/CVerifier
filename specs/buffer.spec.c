/**
 * buffer.spec.c - 缓冲区验证规约示例
 *
 * 本文件演示如何使用 C 语言编写验证规约
 * 规约代码独立编译为 libverification.so
 */

#include "verification.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// 与验证引擎的接口
// ============================================================================

// 这些函数将由验证引擎在符号执行时调用
extern bool __cverifier_is_valid_read(void* ptr, size_t size);
extern bool __cverifier_is_valid_write(void* ptr, size_t size);
extern bool __cverifier_is_separated(void* ptr1, void* ptr2, size_t size);

// ============================================================================
// 内存验证函数实现
// ============================================================================

bool verifiable_read(void* ptr, int size) {
    if (ptr == NULL) {
        return false;
    }
    if (size <= 0) {
        return false;
    }
    // 调用验证引擎进行检查
    return __cverifier_is_valid_read(ptr, (size_t)size);
}

bool verifiable_write(void* ptr, int size) {
    if (ptr == NULL) {
        return false;
    }
    if (size <= 0) {
        return false;
    }
    return __cverifier_is_valid_write(ptr, (size_t)size);
}

bool verifiable_pointer(void* ptr) {
    return ptr != NULL;
}

bool verifiable_separated(void* ptr1, void* ptr2, int size) {
    if (ptr1 == NULL || ptr2 == NULL) {
        return false;
    }
    // 检查两个内存区域不重叠
    char* p1 = (char*)ptr1;
    char* p2 = (char*)ptr2;
    return (p1 + size <= p2) || (p2 + size <= p1);
}

// ============================================================================
// 数值验证函数实现
// ============================================================================

bool in_range(int value, int min, int max) {
    return value >= min && value <= max;
}

bool is_positive(int value) {
    return value > 0;
}

bool is_nonzero(int value) {
    return value != 0;
}

bool will_add_overflow(unsigned int a, unsigned int b) {
    return a > UINT_MAX - b;
}

bool will_signed_add_overflow(int a, int b) {
    if (b > 0) {
        return a > INT_MAX - b;
    } else {
        return a < INT_MIN - b;
    }
}

// ============================================================================
// 浮点验证函数实现 (IEEE 754)
// ============================================================================

#include <math.h>
#include <float.h>

bool is_nan(double value) {
    // IEEE 754: NaN != NaN
    return value != value;
}

bool is_infinity(double value) {
    // IEEE 754: Inf - Inf = NaN, 但 Inf == Inf
    return (value == DBL_MAX || value == -DBL_MAX) &&
           !is_nan(value) &&
           (value - value) != 0.0;
}

bool is_finite(double value) {
    return !is_nan(value) && !is_infinity(value);
}

bool will_multiply_overflow(double a, double b) {
    // 简化检测：检查数量级
    if (a == 0.0 || b == 0.0) {
        return false;
    }

    double max_double = DBL_MAX;
    double abs_a = fabs(a);
    double abs_b = fabs(b);

    // 如果两个数都大于 1，检查是否溢出
    if (abs_a > 1.0 && abs_b > 1.0) {
        return abs_a > max_double / abs_b;
    }

    return false;
}

bool will_lose_precision(double large, double small) {
    // 检测: large + small 导致精度损失
    // 如果 |large| >> |small|，small 可能被"吞掉"
    // 对于 double，53 位尾数，2^53 是精度极限

    if (large == 0.0 || small == 0.0) {
        return false;
    }

    double abs_large = fabs(large);
    double abs_small = fabs(small);

    // 精度损失阈值：2^53 * small
    return abs_large > (9007199254740992.0 * abs_small);  // 2^53
}

// ============================================================================
// 断言函数实现
// ============================================================================

#include <stdio.h>

void assert_true(bool condition, const char* msg) {
    if (!condition) {
        fprintf(stderr, "Assertion failed: %s\n", msg);
        // 注意：这里不终止程序，让验证引擎收集更多信息
    }
}

bool verification_check(bool condition, const char* file, int line) {
    if (!condition) {
        fprintf(stderr, "Verification check failed at %s:%d\n", file, line);
        return false;
    }
    return true;
}

// ============================================================================
// 示例：缓冲区操作验证规约
// ============================================================================

/**
 * 验证 strcpy 操作的安全性
 *
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小
 * @return true 如果操作安全，false 否则
 */
bool verify_strcpy_safe(char* dest, const char* src, int dest_size) {
    // 检查前置条件
    if (!verifiable_read(dest, dest_size)) {
        return false;
    }
    if (!verifiable_read(src, 1)) {  // 至少检查第一个字节
        return false;
    }

    // 检查源字符串长度
    int src_len = 0;
    while (src[src_len] != '\0') {
        src_len++;
        if (src_len >= dest_size) {
            // 源字符串太长，会溢出
            return false;
        }
    }

    return true;
}

/**
 * 验证 memcpy 操作的安全性
 *
 * @param dest 目标缓冲区
 * @param src 源缓冲区
 * @param n 字节数
 * @return true 如果操作安全，false 否则
 */
bool verify_memcpy_safe(void* dest, const void* src, int n) {
    // 检查前置条件
    if (!verifiable_read(src, n)) {
        return false;
    }
    if (!verifiable_write(dest, n)) {
        return false;
    }
    if (!verifiable_separated(dest, (void*)src, n)) {
        return false;
    }

    return true;
}

/**
 * 验证数组访问的安全性
 *
 * @param array 数组
 * @param size 数组大小（元素个数）
 * @param index 访问索引
 * @return true 如果访问安全，false 否则
 */
bool verify_array_access(void* array, int size, int index) {
    // 检查数组有效性
    if (!verifiable_read(array, size)) {
        return false;
    }

    // 检查索引范围
    if (!in_range(index, 0, size - 1)) {
        return false;
    }

    return true;
}

// ============================================================================
// 测试用例规约示例
// ============================================================================

/*@
  requires \valid_read(src + (0 .. size-1));
  requires \valid(dest);
  requires size > 0;
  requires \separated(dest, src);
  assigns dest[0 .. size-1];

  ensures \result == 0 || \result == 1;
*/
int safe_copy_example(char* dest, const char* src, int size) {
    if (!verify_memcpy_safe(dest, src, size)) {
        return 0;  // 失败
    }
    memcpy(dest, src, size);
    return 1;  // 成功
}

/*@
  requires \valid_read(buffer);
  requires size >= 10;
  requires index >= 0 && index < size;

  ensures \result == 0;
*/
int safe_array_access_example(int* buffer, int size, int index) {
    if (!verify_array_access(buffer, size, index)) {
        return -1;
    }
    buffer[index] = 42;
    return 0;
}
