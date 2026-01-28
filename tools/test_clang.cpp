/**
 * @file test_clang.cpp
 * @brief Clang 前端测试程序
 */

#include "cverifier/ClangParser.h"
#include "cverifier/SymbolicExecutionEngine.h"
#include "cverifier/CFG.h"
#include "cverifier/Utils.h"
#include <iostream>
#include <fstream>

using namespace cverifier;
using namespace cverifier::frontend;

/**
 * @brief 测试1：解析简单的C代码
 */
void test1_SimpleFunction() {
    std::cout << "=== Test 1: Simple Function ===" << std::endl;

#ifdef HAVE_LLVM
    // 创建测试代码
    const char* testCode = R"(
int add(int a, int b) {
    int result = a + b;
    return result;
}
    )";

    std::cout << "Test code:" << std::endl;
    std::cout << testCode << std::endl;

    // 注意：这里需要实现 parseCode 或使用临时文件
    std::cout << "\nNote: Full Clang parsing requires file input" << std::endl;
    std::cout << "See test_clang_file.cpp for file-based testing" << std::endl;
#else
    std::cout << "LLVM/Clang not available, skipping test" << std::endl;
#endif

    std::cout << std::endl;
}

/**
 * @brief 测试2：解析控制流
 */
void test2_ControlFlow() {
    std::cout << "=== Test 2: Control Flow ===" << std::endl;

#ifdef HAVE_LLVM
    const char* testCode = R"(
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
    )";

    std::cout << "Test code:" << std::endl;
    std::cout << testCode << std::endl;
    std::cout << "\nNote: Full Clang parsing requires file input" << std::endl;
#else
    std::cout << "LLVM/Clang not available, skipping test" << std::endl;
#endif

    std::cout << std::endl;
}

/**
 * @brief 测试3：解析循环
 */
void test3_Loops() {
    std::cout << "=== Test 3: Loops ===" << std::endl;

#ifdef HAVE_LLVM
    const char* testCode = R"(
int sum_array(int* arr, int n) {
    int sum = 0;
    int i = 0;
    while (i < n) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}
    )";

    std::cout << "Test code:" << std::endl;
    std::cout << testCode << std::endl;
    std::cout << "\nNote: Full Clang parsing requires file input" << std::endl;
#else
    std::cout << "LLVM/Clang not available, skipping test" << std::endl;
#endif

    std::cout << std::endl;
}

/**
 * @brief 测试4：解析包含漏洞的代码
 */
void test4_VulnerableCode() {
    std::cout << "=== Test 4: Vulnerable Code Detection ===" << std::endl;

#ifdef HAVE_LLVM
    const char* testCode = R"(
void vulnerable_function(char* input) {
    char buffer[10];
    int i = 0;

    // Buffer overflow vulnerability
    while (input[i] != '\0') {
        buffer[i] = input[i];
        i = i + 1;
    }

    buffer[i] = '\0';
}
    )";

    std::cout << "Test code:" << std::endl;
    std::cout << testCode << std::endl;
    std::cout << "\nNote: Full Clang parsing requires file input" << std::endl;
#else
    std::cout << "LLVM/Clang not available, skipping test" << std::endl;
#endif

    std::cout << std::endl;
}

/**
 * @brief 测试5：从文件解析并运行符号执行
 */
void test5_ParseFromFile(const std::string& filename) {
    std::cout << "=== Test 5: Parse from File ===" << std::endl;

#ifdef HAVE_LLVM
    std::cout << "Parsing file: " << filename << std::endl;

    // 创建 Clang 解析器
    ClangParser parser;

    // 解析文件
    LLIRModule* module = parser.parseFile(filename);

    if (!module) {
        std::cout << "Failed to parse file: " << parser.getLastError() << std::endl;
        return;
    }

    std::cout << "\nLLIR Module created successfully!" << std::endl;
    std::cout << "Module name: " << module->getName() << std::endl;
    std::cout << "Number of functions: " << module->getFunctions().size() << std::endl;

    // 打印每个函数的信息
    for (auto* func : module->getFunctions()) {
        std::cout << "\n  Function: " << func->getName() << std::endl;
        std::cout << "    Basic blocks: " << func->getBasicBlocks().size() << std::endl;

        // 创建CFG
        CFG cfg(func);
        std::cout << "    CFG nodes: " << cfg.getNodes().size() << std::endl;

        // 打印CFG
        std::cout << "\n" << cfg.toString() << std::endl;

        // 运行符号执行
        std::cout << "\n  Running symbolic execution..." << std::endl;
        SymbolicExecutionConfig config;
        config.maxDepth = 10;
        config.verbose = true;

        SymbolicExecutionEngine engine(module, config);
        engine.runOnFunction(func->getName());

        std::cout << "\n" << engine.getStatistics() << std::endl;
    }

    // 清理
    delete module;
#else
    std::cout << "LLVM/Clang not available, skipping test" << std::endl;
#endif

    std::cout << std::endl;
}

/**
 * @brief 创建测试C文件
 */
void createTestFile(const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to create test file: " << filename << std::endl;
        return;
    }

    file << R"(/**
 * Test file for CVerifier Clang frontend
 */

#include <stdio.h>

// Simple addition function
int add(int a, int b) {
    int result = a + b;
    return result;
}

// Function with control flow
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

// Function with loop
int sum_array(int* arr, int n) {
    int sum = 0;
    int i = 0;

    while (i < n) {
        sum = sum + arr[i];
        i = i + 1;
    }

    return sum;
}

// Vulnerable function
void vulnerable_function(char* input) {
    char buffer[10];
    int i = 0;

    while (input[i] != '\0') {
        buffer[i] = input[i];  // Buffer overflow!
        i = i + 1;
    }

    buffer[i] = '\0';
}

// Main function
int main() {
    int x = 5;
    int y = 10;

    int z = add(x, y);
    int m = max(x, y);

    int arr[] = {1, 2, 3, 4, 5};
    int s = sum_array(arr, 5);

    printf("add: %d\n", z);
    printf("max: %d\n", m);
    printf("sum: %d\n", s);

    return 0;
}
)";

    file.close();
    std::cout << "Test file created: " << filename << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "CVerifier Clang Frontend Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;

    // 设置日志级别
    utils::Logger::setLevel(utils::Logger::Level::Info);

#ifdef HAVE_LLVM
    std::cout << "LLVM/Clang: Available" << std::endl;
    std::cout << std::endl;
#else
    std::cout << "LLVM/Clang: Not Available" << std::endl;
    std::cout << "Install LLVM/Clang 15+ to enable Clang frontend" << std::endl;
    std::cout << std::endl;
#endif

    // 运行基础测试
    test1_SimpleFunction();
    test2_ControlFlow();
    test3_Loops();
    test4_VulnerableCode();

    // 文件解析测试
#ifdef HAVE_LLVM
    if (argc > 1) {
        // 解析用户指定的文件
        std::string filename = argv[1];
        test5_ParseFromFile(filename);
    } else {
        // 创建并解析测试文件
        std::string testFile = "/tmp/test_cverifier.c";
        createTestFile(testFile);
        test5_ParseFromFile(testFile);

        std::cout << "\nYou can also test with your own files:" << std::endl;
        std::cout << "  " << argv[0] << " <path-to-c-file>" << std::endl;
    }
#endif

    std::cout << "=====================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;

    return 0;
}
