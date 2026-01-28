/**
 * @file test_memory_leak.c
 * @brief 内存泄漏检测测试用例
 *
 * 这些测试用例用于验证内存泄漏检测器的功能
 */

#include <stdlib.h>
#include <string.h>

// ============================================================================
// 测试用例 1: 简单内存泄漏（应该检测到）
// ============================================================================
void test_simple_leak() {
    int* ptr = (int*)malloc(sizeof(int));
    *ptr = 42;
    // BUG: 忘记 free(ptr)
}

// ============================================================================
// 测试用例 2: 正确释放（不应报错）
// ============================================================================
void test_no_leak() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        free(ptr);  // OK: 正确释放
    }
}

// ============================================================================
// 测试用例 3: 循环中的内存泄漏（应该检测到）
// ============================================================================
void test_leak_in_loop() {
    for (int i = 0; i < 10; i++) {
        int* ptr = (int*)malloc(sizeof(int));
        *ptr = i;
        // BUG: 循环中每次分配都未释放
    }
}

// ============================================================================
// 测试用例 4: 循环中正确释放（不应报错）
// ============================================================================
void test_no_leak_in_loop() {
    for (int i = 0; i < 10; i++) {
        int* ptr = (int*)malloc(sizeof(int));
        if (ptr) {
            *ptr = i;
            free(ptr);  // OK: 每次都释放
        }
    }
}

// ============================================================================
// 测试用例 5: 条件分支中的泄漏（应该检测到）
// ============================================================================
void test_leak_in_branch() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        if (ptr != NULL) {
            // BUG: 某些路径未释放
            return;
        }
        free(ptr);
    }
}

// ============================================================================
// 测试用例 6: 函数返回泄漏的指针（应该检测到）
// ============================================================================
int* allocate_and_leak() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
    }
    return ptr;  // 调用者负责释放
}

void test_leak_from_function() {
    int* ptr = allocate_and_leak();
    if (ptr) {
        printf("Value: %d\n", *ptr);
        // BUG: 未释放 ptr
    }
}

// ============================================================================
// 测试用例 7: 函数返回后正确释放（不应报错）
// ============================================================================
void test_no_leak_from_function() {
    int* ptr = allocate_and_leak();
    if (ptr) {
        printf("Value: %d\n", *ptr);
        free(ptr);  // OK: 调用者正确释放
    }
}

// ============================================================================
// 测试用例 8: realloc 失败导致泄漏（应该检测到）
// ============================================================================
void test_realloc_leak() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        int* new_ptr = (int*)realloc(ptr, sizeof(int) * 2);
        // 如果 realloc 失败，ptr 仍然有效但未释放
        if (new_ptr == NULL) {
            // BUG: 未释放原始 ptr
            return;
        }
        // 继续使用 new_ptr
        free(new_ptr);
    }
}

// ============================================================================
// 测试用例 9: realloc 成功（不应报错）
// ============================================================================
void test_realloc_no_leak() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        int* new_ptr = (int*)realloc(ptr, sizeof(int) * 2);
        if (new_ptr) {
            new_ptr[0] = 42;
            free(new_ptr);  // OK: 释放 realloc 后的指针
        } else {
            free(ptr);  // OK: realloc 失败，释放原始指针
        }
    }
}

// ============================================================================
// 测试用例 10: 字符串泄漏（应该检测到）
// ============================================================================
void test_string_leak() {
    char* str = (char*)malloc(100);
    if (str) {
        strcpy(str, "Hello, World!");
        // BUG: 未释放 str
    }
}

// ============================================================================
// 测试用例 11: 结构体泄漏（应该检测到）
// ============================================================================
struct Data {
    int* array;
    size_t size;
};

void test_struct_leak() {
    struct Data* data = (struct Data*)malloc(sizeof(struct Data));
    if (data) {
        data->size = 10;
        data->array = (int*)malloc(sizeof(int) * data->size);
        if (data->array) {
            for (size_t i = 0; i < data->size; i++) {
                data->array[i] = i;
            }
        }
        // BUG: 未释放 data->array 和 data
    }
}

// ============================================================================
// 测试用例 12: 结构体正确释放（不应报错）
// ============================================================================
void test_struct_no_leak() {
    struct Data* data = (struct Data*)malloc(sizeof(struct Data));
    if (data) {
        data->size = 10;
        data->array = (int*)malloc(sizeof(int) * data->size);
        if (data->array) {
            for (size_t i = 0; i < data->size; i++) {
                data->array[i] = i;
            }
            free(data->array);  // OK: 释放数组
        }
        free(data);  // OK: 释放结构体
    }
}

// ============================================================================
// 测试用例 13: 多次分配部分释放（应该检测到）
// ============================================================================
void test_partial_leak() {
    int* ptr1 = (int*)malloc(sizeof(int));
    int* ptr2 = (int*)malloc(sizeof(int));
    int* ptr3 = (int*)malloc(sizeof(int));

    if (ptr1) *ptr1 = 1;
    if (ptr2) *ptr2 = 2;
    if (ptr3) *ptr3 = 3;

    free(ptr1);  // OK: 释放 ptr1
    // BUG: ptr2 和 ptr3 未释放
}

// ============================================================================
// 测试用例 14: 所有分配都释放（不应报错）
// ============================================================================
void test_no_leak_multiple() {
    int* ptr1 = (int*)malloc(sizeof(int));
    int* ptr2 = (int*)malloc(sizeof(int));
    int* ptr3 = (int*)malloc(sizeof(int));

    if (ptr1) *ptr1 = 1;
    if (ptr2) *ptr2 = 2;
    if (ptr3) *ptr3 = 3;

    if (ptr1) free(ptr1);  // OK
    if (ptr2) free(ptr2);  // OK
    if (ptr3) free(ptr3);  // OK
}

// ============================================================================
// 测试用例 15: exit() 时的泄漏（可配置是否报告）
// ============================================================================
void test_leak_before_exit() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        // BUG: 程序退出前未释放
    }
    exit(0);
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char** argv) {
    if (argc > 1) {
        int test_case = atoi(argv[1]);
        switch (test_case) {
            case 1: test_simple_leak(); break;
            case 2: test_no_leak(); break;
            case 3: test_leak_in_loop(); break;
            case 4: test_no_leak_in_loop(); break;
            case 5: test_leak_in_branch(); break;
            case 6: test_leak_from_function(); break;
            case 7: test_no_leak_from_function(); break;
            case 8: test_realloc_leak(); break;
            case 9: test_realloc_no_leak(); break;
            case 10: test_string_leak(); break;
            case 11: test_struct_leak(); break;
            case 12: test_struct_no_leak(); break;
            case 13: test_partial_leak(); break;
            case 14: test_no_leak_multiple(); break;
            case 15: test_leak_before_exit(); break;
            default: return 1;
        }
    }

    return 0;
}
