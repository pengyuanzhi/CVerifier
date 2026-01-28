#ifndef VERIFICATION_H
#define VERIFICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// 内存验证函数
// ============================================================================

/**
 * 验证内存可读性
 * @param ptr 内存指针
 * @param size 字节数
 * @return true 如果内存可读，false 否则
 */
bool verifiable_read(void* ptr, int size);

/**
 * 验证内存可写性
 * @param ptr 内存指针
 * @param size 字节数
 * @return true 如果内存可写，false 否则
 */
bool verifiable_write(void* ptr, int size);

/**
 * 验证指针有效性（非NULL且在合法范围内）
 * @param ptr 指针
 * @return true 如果指针有效，false 否则
 */
bool verifiable_pointer(void* ptr);

/**
 * 验证两块内存不重叠
 * @param ptr1 第一块内存
 * @param ptr2 第二块内存
 * @param size 内存大小
 * @return true 如果内存分离，false 否则
 */
bool verifiable_separated(void* ptr1, void* ptr2, int size);

// ============================================================================
// 数值验证函数
// ============================================================================

/**
 * 验证值在指定范围内
 * @param value 待验证的值
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return true 如果在范围内，false 否则
 */
bool in_range(int value, int min, int max);

/**
 * 验证无符号整数为正数
 * @param value 待验证的值
 * @return true 如果为正数，false 否则
 */
bool is_positive(int value);

/**
 * 验证整数为非零
 * @param value 待验证的值
 * @return true 如果非零，false 否则
 */
bool is_nonzero(int value);

/**
 * 验证无符号整型溢出
 * @param a 操作数1
 * @param b 操作数2
 * @return true 如果 a + b 会溢出，false 否则
 */
bool will_add_overflow(unsigned int a, unsigned int b);

/**
 * 验证有符号整型溢出
 * @param a 操作数1
 * @param b 操作数2
 * @return true 如果 a + b 会溢出，false 否则
 */
bool will_signed_add_overflow(int a, int b);

// ============================================================================
// 浮点验证函数 (IEEE 754)
// ============================================================================

/**
 * 检测浮点数是否为 NaN
 * @param value 浮点数值
 * @return true 如果是 NaN，false 否则
 */
bool is_nan(double value);

/**
 * 检测浮点数是否为无穷大
 * @param value 浮点数值
 * @return true 如果是无穷大，false 否则
 */
bool is_infinity(double value);

/**
 * 检测浮点数是否为有限数（非 NaN，非 Inf）
 * @param value 浮点数值
 * @return true 如果是有限数，false 否则
 */
bool is_finite(double value);

/**
 * 检测浮点乘法是否会上溢
 * @param a 操作数1
 * @param b 操作数2
 * @return true 如果 a * b 会溢出到 Inf，false 否则
 */
bool will_multiply_overflow(double a, double b);

/**
 * 检测浮点运算精度损失
 * @param large 大数
 * @param small 小数
 * @return true 如果 large + small 会导致精度损失，false 否则
 */
bool will_lose_precision(double large, double small);

// ============================================================================
// 断言函数
// ============================================================================

/**
 * 运行时断言
 * @param condition 条件
 * @param msg 错误消息
 */
void assert_true(bool condition, const char* msg);

/**
 * 静态断言（编译时）
 * @param condition 编译时常量
 */
#define STATIC_ASSERT(condition) _Static_assert(condition, #condition)

/**
 * 运行时检查（规约验证专用）
 * @param condition 条件
 * @param file 源文件
 * @param line 行号
 * @return true 如果条件成立，false 否则（不终止程序）
 */
bool verification_check(bool condition, const char* file, int line);

#define VERIFICATION_CHECK(cond) \
    verification_check(cond, __FILE__, __LINE__)

// ============================================================================
// ACSL 风格谓词（C语言实现）
// ============================================================================

/**
 * ACSL \valid 谓词实现
 * 检查指针指向的内存区域可读
 */
#define valid(ptr) verifiable_read(ptr, sizeof(*(ptr)))

/**
 * ACSL \valid_read 谓词实现
 */
#define valid_read(ptr) verifiable_read(ptr, sizeof(*(ptr)))

/**
 * ACSL \separated 谓词实现
 * 检查两个指针指向的内存不重叠
 */
#define separated(ptr1, ptr2) \
    verifiable_separated(ptr1, ptr2, sizeof(*(ptr1)))

/**
 * ACSL \null 谓词实现
 */
#define null(ptr) ((ptr) == NULL)

/**
 * ACSL \initialized 谓词实现
 * 检查指针是否已初始化（非 NULL）
 */
#define initialized(ptr) ((ptr) != NULL)

#ifdef __cplusplus
}
#endif

#endif // VERIFICATION_H
