/**
 * @file test_null_pointer.c
 * @brief 空指针解引用检测测试用例
 *
 * 这些测试用例用于验证空指针解引用检测器的功能
 */

#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// 测试用例 1: 显式 NULL 解引用（应该检测到）
// ============================================================================
void test_explicit_null_dereference() {
    int* ptr = NULL;
    *ptr = 42;  // BUG: 解引用 NULL 指针
}

// ============================================================================
// 测试用例 2: 条件检查后解引用（安全，不应报错）
// ============================================================================
void test_checked_null_dereference() {
    int* ptr = NULL;
    if (ptr != NULL) {
        *ptr = 42;  // OK: 已检查
    }
}

// ============================================================================
// 测试用例 3: 未初始化指针可能为 NULL（应该检测到）
// ============================================================================
void test_uninitialized_pointer() {
    int* ptr;  // 未初始化
    *ptr = 42;  // BUG: ptr 可能包含随机值或 NULL
}

// ============================================================================
// 测试用例 4: malloc 失败后使用（应该检测到）
// ============================================================================
void test_malloc_failure() {
    int* ptr = (int*)malloc(sizeof(int) * 1000000000);  // 可能失败
    *ptr = 42;  // BUG: 如果 malloc 失败，ptr 为 NULL
    free(ptr);
}

// ============================================================================
// 测试用例 5: malloc 成功检查（安全，不应报错）
// ============================================================================
void test_malloc_checked() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr != NULL) {
        *ptr = 42;  // OK: 已检查
        free(ptr);
    }
}

// ============================================================================
// 测试用例 6: 函数返回 NULL 后使用（应该检测到）
// ============================================================================
int* get_null_ptr() {
    return NULL;
}

void test_function_return_null() {
    int* ptr = get_null_ptr();
    *ptr = 42;  // BUG: ptr 为 NULL
}

// ============================================================================
// 测试用例 7: 通过指针传递 NULL（应该检测到）
// ============================================================================
void set_value(int* ptr) {
    *ptr = 42;  // BUG: ptr 可能为 NULL
}

void test_pass_null_to_function() {
    set_value(NULL);  // 传递 NULL
}

// ============================================================================
// 测试用例 8: 结构体指针解引用 NULL（应该检测到）
// ============================================================================
struct Point {
    int x;
    int y;
};

void test_struct_null_dereference() {
    struct Point* p = NULL;
    p->x = 10;  // BUG: 解引用 NULL
}

// ============================================================================
// 测试用例 9: 数组指针 NULL（应该检测到）
// ============================================================================
void test_array_null_dereference() {
    int* arr = NULL;
    arr[0] = 42;  // BUG: arr 为 NULL
}

// ============================================================================
// 测试用例 10: 释放后使用（Use-After-Free）（应该检测到）
// ============================================================================
void test_use_after_free() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        free(ptr);
        *ptr = 43;  // BUG: 使用已释放的指针
    }
}

// ============================================================================
// 测试用例 11: 双重释放（Double Free）（应该检测到）
// ============================================================================
void test_double_free() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        free(ptr);
        free(ptr);  // BUG: 双重释放
    }
}

// ============================================================================
// 测试用例 12: 安全的指针使用（不应报错）
// ============================================================================
void test_safe_pointer_usage() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr != NULL) {
        *ptr = 42;
        printf("Value: %d\n", *ptr);
        free(ptr);
        ptr = NULL;  // 防止悬空指针
    }
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char** argv) {
    if (argc > 1) {
        int test_case = atoi(argv[1]);
        switch (test_case) {
            case 1: test_explicit_null_dereference(); break;
            case 2: test_checked_null_dereference(); break;
            case 3: test_uninitialized_pointer(); break;
            case 4: test_malloc_failure(); break;
            case 5: test_malloc_checked(); break;
            case 6: test_function_return_null(); break;
            case 7: test_pass_null_to_function(); break;
            case 8: test_struct_null_dereference(); break;
            case 9: test_array_null_dereference(); break;
            case 10: test_use_after_free(); break;
            case 11: test_double_free(); break;
            case 12: test_safe_pointer_usage(); break;
            default: return 1;
        }
    }

    return 0;
}
