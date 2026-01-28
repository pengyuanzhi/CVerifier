/**
 * @file test_integer_overflow.c
 * @brief 整数溢出检测测试用例
 *
 * 这些测试用例用于验证整数溢出检测器的功能
 */

#include <limits.h>
#include <stdlib.h>

// ============================================================================
// 测试用例 1: 无符号整数加法溢出（应该检测到）
// ============================================================================
void test_unsigned_add_overflow() {
    unsigned int x = UINT_MAX;
    unsigned int y = 1;
    unsigned int result = x + y;  // BUG: 无符号加法溢出
}

// ============================================================================
// 测试用例 2: 无符号整数加法安全（不应报错）
// ============================================================================
void test_unsigned_add_safe() {
    unsigned int x = 100;
    unsigned int y = 50;
    unsigned int result = x + y;  // OK: 150 < UINT_MAX
}

// ============================================================================
// 测试用例 3: 有符号整数加法溢出（应该检测到）
// ============================================================================
void test_signed_add_overflow() {
    int x = INT_MAX;
    int y = 1;
    int result = x + y;  // BUG: 有符号加法溢出（未定义行为）
}

// ============================================================================
// 测试用例 4: 有符号整数下溢（应该检测到）
// ============================================================================
void test_signed_underflow() {
    int x = INT_MIN;
    int y = 1;
    int result = x - y;  // BUG: 有符号减法下溢
}

// ============================================================================
// 测试用例 5: 无符号整数减法下溢（应该检测到）
// ============================================================================
void test_unsigned_sub_underflow() {
    unsigned int x = 0;
    unsigned int y = 1;
    unsigned int result = x - y;  // BUG: 无符号减法下溢（回绕）
}

// ============================================================================
// 测试用例 6: 乘法溢出（应该检测到）
// ============================================================================
void test_multiplication_overflow() {
    int x = INT_MAX / 2;
    int y = 3;
    int result = x * y;  // BUG: 乘法溢出
}

// ============================================================================
// 测试用例 7: 左移溢出（应该检测到）
// ============================================================================
void test_left_shift_overflow() {
    int x = 1;
    int result = x << 31;  // BUG: 左移溢出（有符号整数）
}

// ============================================================================
// 测试用例 8: 无符号左移安全（不应报错）
// ============================================================================
void test_unsigned_left_shift_safe() {
    unsigned int x = 1;
    unsigned int result = x << 16;  // OK: 结果在范围内
}

// ============================================================================
// 测试用例 9: 类型转换截断（应该检测到）
// ============================================================================
void test_type_conversion_truncation() {
    long x = LONG_MAX;
    int y = (int)x;  // BUG: 长整型转整型截断
}

// ============================================================================
// 测试用例 10: 安全类型转换（不应报错）
// ============================================================================
void test_safe_type_conversion() {
    long x = 1000;
    int y = (int)x;  // OK: 1000 在 int 范围内
}

// ============================================================================
// 测试用例 11: malloc 大小溢出（应该检测到）
// ============================================================================
void test_malloc_size_overflow() {
    size_t count = SIZE_MAX / sizeof(int);
    size_t element_size = sizeof(int);
    size_t total = count * element_size;
    int* ptr = (int*)malloc(total);  // BUG: 大小计算溢出
    if (ptr) {
        free(ptr);
    }
}

// ============================================================================
// 测试用例 12: 数组索引溢出（应该检测到）
// ============================================================================
void test_array_index_overflow() {
    int arr[10];
    unsigned int index = UINT_MAX;
    // 如果 index 被用作数组索引，可能导致问题
    int value = arr[index % 10];  // OK: 取模后安全
}

// ============================================================================
// 测试用例 13: 循环计数器溢出（应该检测到）
// ============================================================================
void test_loop_counter_overflow() {
    for (unsigned int i = UINT_MAX - 5; i < UINT_MAX; i++) {
        // 当 i 溢出时变为 0，可能导致无限循环
        // BUG: 循环计数器溢出
    }
}

// ============================================================================
// 测试用例 14: 除法前的溢出检查（安全，不应报错）
// ============================================================================
void test_safe_division() {
    int x = 100;
    int y = 5;
    if (y != 0) {
        int result = x / y;  // OK: 已检查除零
    }
}

// ============================================================================
// 测试用例 15: 取模运算溢出（应该检测到）
// ============================================================================
void test_modulo_overflow() {
    int x = INT_MIN;
    int y = -1;
    int result = x % y;  // BUG: INT_MIN % -1 溢出（某些平台）
}

// ============================================================================
// 测试用例 16: 一元负号溢出（应该检测到）
// ============================================================================
void test_unary_minus_overflow() {
    int x = INT_MIN;
    int result = -x;  // BUG: -INT_MIN 溢出（未定义行为）
}

// ============================================================================
// 测试用例 17: 绝对值溢出（应该检测到）
// ============================================================================
void test_abs_overflow() {
    int x = INT_MIN;
    int result = abs(x);  // BUG: abs(INT_MIN) 溢出
}

// ============================================================================
// 测试用例 18: 比较运算中的溢出（应该检测到）
// ============================================================================
void test_comparison_overflow() {
    unsigned int x = 10;
    int y = -1;
    if (x > y) {
        // BUG: 有符号/无符号比较导致意外的行为
        // y 被转换为无符号数（非常大的正数）
    }
}

// ============================================================================
// 测试用例 19: 累加器溢出（应该检测到）
// ============================================================================
void test_accumulator_overflow() {
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += INT_MAX / 500;  // BUG: 累加溢出
    }
}

// ============================================================================
// 测试用例 20: 安全的算术运算（不应报错）
// ============================================================================
void test_safe_arithmetic() {
    int x = 100;
    int y = 50;
    int add_result = x + y;
    int sub_result = x - y;
    int mul_result = x * y;
    int div_result = x / y;

    unsigned int ux = 100;
    unsigned int uy = 50;
    unsigned int uadd_result = ux + uy;

    // OK: 所有运算都在安全范围内
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char** argv) {
    if (argc > 1) {
        int test_case = atoi(argv[1]);
        switch (test_case) {
            case 1: test_unsigned_add_overflow(); break;
            case 2: test_unsigned_add_safe(); break;
            case 3: test_signed_add_overflow(); break;
            case 4: test_signed_underflow(); break;
            case 5: test_unsigned_sub_underflow(); break;
            case 6: test_multiplication_overflow(); break;
            case 7: test_left_shift_overflow(); break;
            case 8: test_unsigned_left_shift_safe(); break;
            case 9: test_type_conversion_truncation(); break;
            case 10: test_safe_type_conversion(); break;
            case 11: test_malloc_size_overflow(); break;
            case 12: test_array_index_overflow(); break;
            case 13: test_loop_counter_overflow(); break;
            case 14: test_safe_division(); break;
            case 15: test_modulo_overflow(); break;
            case 16: test_unary_minus_overflow(); break;
            case 17: test_abs_overflow(); break;
            case 18: test_comparison_overflow(); break;
            case 19: test_accumulator_overflow(); break;
            case 20: test_safe_arithmetic(); break;
            default: return 1;
        }
    }

    return 0;
}
