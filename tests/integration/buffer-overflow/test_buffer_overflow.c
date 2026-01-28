/**
 * @file test_buffer_overflow.c
 * @brief 缓冲区溢出检测测试用例
 *
 * 这些测试用例用于验证缓冲区溢出检测器的功能
 */

#include <string.h>
#include <stdlib.h>

// ============================================================================
// 测试用例 1: 静态数组越界（应该检测到）
// ============================================================================
void test_static_array_overflow() {
    char buffer[10];
    buffer[10] = 'a';  // BUG: 数组越界访问
}

// ============================================================================
// 测试用例 2: 静态数组安全访问（不应报错）
// ============================================================================
void test_static_array_safe() {
    char buffer[10];
    for (int i = 0; i < 10; i++) {
        buffer[i] = 'a';  // OK: 在范围内
    }
}

// ============================================================================
// 测试用例 3: 栈溢出（应该检测到）
// ============================================================================
void test_stack_overflow() {
    int size = 20;
    char buffer[10];
    for (int i = 0; i <= size; i++) {
        buffer[i] = 'a';  // BUG: 当 i >= 10 时溢出
    }
}

// ============================================================================
// 测试用例 4: 字符串溢出（strcpy）（应该检测到）
// ============================================================================
void test_string_overflow() {
    char buffer[10];
    char* input = "This is a very long string that exceeds buffer size";
    strcpy(buffer, input);  // BUG: 缓冲区溢出
}

// ============================================================================
// 测试用例 5: 字符串安全使用（strncpy）（不应报错）
// ============================================================================
void test_string_safe() {
    char buffer[10];
    char* input = "Hello";
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';  // OK: 安全使用
}

// ============================================================================
// 测试用例 6: 堆溢出（应该检测到）
// ============================================================================
void test_heap_overflow() {
    char* buffer = (char*)malloc(10);
    if (buffer) {
        buffer[10] = 'a';  // BUG: 堆溢出
        free(buffer);
    }
}

// ============================================================================
// 测试用例 7: 堆安全访问（不应报错）
// ============================================================================
void test_heap_safe() {
    char* buffer = (char*)malloc(10);
    if (buffer) {
        for (int i = 0; i < 10; i++) {
            buffer[i] = 'a';  // OK: 在范围内
        }
        free(buffer);
    }
}

// ============================================================================
// 测试用例 8: 条件溢出（可能检测到）
// ============================================================================
void test_conditional_overflow(int x) {
    char buffer[10];
    if (x > 5) {
        buffer[x] = 'a';  // 可能的 BUG: 当 x >= 10 时溢出
    }
}

// ============================================================================
// 测试用例 9: 数组索引为负（应该检测到）
// ============================================================================
void test_negative_index() {
    char buffer[10];
    int index = -1;
    buffer[index] = 'a';  // BUG: 负索引
}

// ============================================================================
// 测试用例 10: 指针算术溢出（应该检测到）
// ============================================================================
void test_pointer_arithmetic_overflow() {
    char buffer[10];
    char* ptr = buffer;
    ptr = ptr + 15;  // BUG: 指针超出范围
    *ptr = 'a';
}

// ============================================================================
// 主函数（运行所有测试）
// ============================================================================
int main(int argc, char** argv) {
    // 注意：这些函数包含故意的bug用于测试
    // 在实际测试中，应该单独调用每个函数

    if (argc > 1) {
        int test_case = atoi(argv[1]);
        switch (test_case) {
            case 1: test_static_array_overflow(); break;
            case 2: test_static_array_safe(); break;
            case 3: test_stack_overflow(); break;
            case 4: test_string_overflow(); break;
            case 5: test_string_safe(); break;
            case 6: test_heap_overflow(); break;
            case 7: test_heap_safe(); break;
            case 8: test_conditional_overflow(10); break;
            case 9: test_negative_index(); break;
            case 10: test_pointer_arithmetic_overflow(); break;
            default: return 1;
        }
    }

    return 0;
}
