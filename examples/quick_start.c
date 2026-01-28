/**
 * @file quick_start.c
 * @brief CVerifier 快速入门示例
 *
 * 这个文件包含了几个常见的安全漏洞示例，
 * 用于演示 CVerifier 的基本功能。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 示例 1: 缓冲区溢出
// ============================================================================
void buffer_overflow_example() {
    char buffer[10];
    char* input = "This string is too long!";

    // ❌ BUG: 缓冲区溢出
    strcpy(buffer, input);

    printf("Buffer: %s\n", buffer);
}

// ✅ 安全版本
void buffer_overflow_safe() {
    char buffer[10];
    char* input = "Hello";

    // ✅ 安全: 使用 strncpy 并限制长度
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    printf("Buffer: %s\n", buffer);
}

// ============================================================================
// 示例 2: 空指针解引用
// ============================================================================
void null_pointer_example() {
    int* ptr = NULL;

    // ❌ BUG: 空指针解引用
    *ptr = 42;
}

// ✅ 安全版本
void null_pointer_safe() {
    int* ptr = NULL;

    // ✅ 安全: 检查指针是否为空
    if (ptr != NULL) {
        *ptr = 42;
    } else {
        ptr = (int*)malloc(sizeof(int));
        if (ptr) {
            *ptr = 42;
            printf("Value: %d\n", *ptr);
            free(ptr);
        }
    }
}

// ============================================================================
// 示例 3: 内存泄漏
// ============================================================================
void memory_leak_example() {
    int* ptr = (int*)malloc(sizeof(int));
    *ptr = 42;

    // ❌ BUG: 忘记释放内存
    // free(ptr);  // 这行被注释掉了
}

// ✅ 安全版本
void memory_leak_safe() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        printf("Value: %d\n", *ptr);
        free(ptr);  // ✅ 正确释放
        ptr = NULL; // ✅ 防止悬空指针
    }
}

// ============================================================================
// 示例 4: 整数溢出
// ============================================================================
void integer_overflow_example() {
    unsigned int x = 0xFFFFFFFF;
    unsigned int y = 1;

    // ❌ BUG: 无符号整数加法溢出
    unsigned int result = x + y;

    printf("Result: %u\n", result);
}

// ✅ 安全版本
void integer_overflow_safe() {
    unsigned int x = 0xFFFFFFFF;
    unsigned int y = 1;

    // ✅ 安全: 检查溢出
    if (x <= UINT_MAX - y) {
        unsigned int result = x + y;
        printf("Result: %u\n", result);
    } else {
        printf("Error: Integer overflow!\n");
    }
}

// ============================================================================
// 示例 5: Use-After-Free
// ============================================================================
void use_after_free_example() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        free(ptr);

        // ❌ BUG: 使用已释放的内存
        *ptr = 43;
    }
}

// ✅ 安全版本
void use_after_free_safe() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        *ptr = 42;
        printf("Value: %d\n", *ptr);
        free(ptr);
        ptr = NULL;  // ✅ 释放后置为 NULL

        // ✅ 现在 ptr 是 NULL，不会误用
        if (ptr != NULL) {
            *ptr = 43;
        }
    }
}

// ============================================================================
// 示例 6: 双重释放
// ============================================================================
void double_free_example() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        free(ptr);
        // ❌ BUG: 双重释放
        free(ptr);
    }
}

// ✅ 安全版本
void double_free_safe() {
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr) {
        free(ptr);
        ptr = NULL;  // ✅ 释放后置为 NULL

        // ✅ 现在 free(NULL) 是安全的
        free(ptr);  // 释放 NULL 是允许的（no-op）
    }
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char** argv) {
    printf("CVerifier Quick Start Examples\n");
    printf("================================\n\n");

    if (argc > 1) {
        int example = atoi(argv[1]);

        switch (example) {
            case 1:
                printf("Example 1: Buffer Overflow\n");
                buffer_overflow_example();
                break;
            case 2:
                printf("Example 2: Null Pointer Dereference\n");
                null_pointer_example();
                break;
            case 3:
                printf("Example 3: Memory Leak\n");
                memory_leak_example();
                break;
            case 4:
                printf("Example 4: Integer Overflow\n");
                integer_overflow_example();
                break;
            case 5:
                printf("Example 5: Use-After-Free\n");
                use_after_free_example();
                break;
            case 6:
                printf("Example 6: Double Free\n");
                double_free_example();
                break;
            default:
                printf("Usage: %s <example_number>\n", argv[0]);
                printf("\nAvailable examples:\n");
                printf("  1 - Buffer Overflow\n");
                printf("  2 - Null Pointer Dereference\n");
                printf("  3 - Memory Leak\n");
                printf("  4 - Integer Overflow\n");
                printf("  5 - Use-After-Free\n");
                printf("  6 - Double Free\n");
                return 1;
        }
    } else {
        printf("Usage: %s <example_number>\n", argv[0]);
        printf("\nRun 'cverifier %s' to detect vulnerabilities!\n", argv[0]);
        return 1;
    }

    return 0;
}
