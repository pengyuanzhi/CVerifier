/**
 * @file cverifier.cpp
 * @brief CVerifier 主程序
 */

#include "cverifier/Core.h"
#include "cverifier/SymbolicExecutionEngine.h"
#include "cverifier/LLIRModule.h"
#include "cverifier/LLIRFactory.h"
#include "cverifier/CFG.h"
#include "cverifier/Utils.h"
#include <iostream>
#include <sstream>

using namespace cverifier;
using namespace cverifier::core;

/**
 * @brief 打印使用说明
 */
void printUsage(const char* programName) {
    std::cout << "CVerifier - C代码形式验证工具 v" << Version::toString() << "\n\n";
    std::cout << "用法: " << programName << " [选项] <输入文件>\n\n";
    std::cout << "选项:\n";
    std::cout << "  --help              显示此帮助信息\n";
    std::cout << "  --version           显示版本信息\n";
    std::cout << "  --verbose           详细输出\n";
    std::cout << "  --config <文件>     指定配置文件\n";
    std::cout << "  --timeout <秒>      设置超时时间（默认300秒）\n";
    std::cout << "  --max-depth <深度>  设置最大探索深度（默认100）\n";
    std::cout << "  --enable-all        启用所有检查器\n";
    std::cout << "  --check-buffer      启用缓冲区溢出检测\n";
    std::cout << "  --check-null        启用空指针检测\n";
    std::cout << "  --check-leak        启用内存泄漏检测\n";
    std::cout << "  --check-overflow    启用整数溢出检测\n";
    std::cout << "\n示例:\n";
    std::cout << "  " << programName << " test.c\n";
    std::cout << "  " << programName << " --verbose --enable-all src/*.c\n";
    std::cout << "  " << programName << " --config=aggressive.yaml main.c\n";
}

/**
 * @brief 打印版本信息
 */
void printVersion() {
    std::cout << "CVerifier v" << Version::toString() << "\n";
    std::cout << "C++ 形式验证工具 - 符号执行 + 抽象解释\n";
    std::cout << "\n";
    std::cout << "特性:\n";
    std::cout << "  - 符号执行引擎\n";
    std::cout << "  - 抽象解释器\n";
    std::cout << "  - 多种漏洞检测器\n";
    std::cout << "  - 路径探索和状态管理\n";
    std::cout << "\n";
    std::cout << "漏洞检测:\n";
    std::cout << "  - 缓冲区溢出\n";
    std::cout << "  - 空指针解引用\n";
    std::cout << "  - 内存泄漏\n";
    std::cout << "  - 整数溢出\n";
}

/**
 * @brief 创建示例LLIR模块用于演示
 */
LLIRModule* createExampleModule() {
    // 创建模块
    auto* module = LLIRFactory::createModule("example");

    // 创建函数
    auto* func = LLIRFactory::createFunction("test_function");
    module->addFunction(func);

    // 创建基本块
    auto* entry = LLIRFactory::createBasicBlock("entry");
    auto* then = LLIRFactory::createBasicBlock("then");
    auto* else_ = LLIRFactory::createBasicBlock("else");
    auto* merge = LLIRFactory::createBasicBlock("merge");

    func->addBasicBlock(entry);
    func->addBasicBlock(then);
    func->addBasicBlock(else_);
    func->addBasicBlock(merge);
    func->setEntryBlock(entry);

    // 创建常量
    auto* const_10 = LLIRFactory::createIntConstant(10);
    auto* const_5 = LLIRFactory::createIntConstant(5);
    auto* const_0 = LLIRFactory::createIntConstant(0);

    // 创建一些指令
    auto* alloca = LLIRFactory::createAlloca(const_10);
    entry->addInstruction(alloca);

    auto* var_x = LLIRFactory::createVariable("x", ValueType::Integer, 0);
    auto* add = LLIRFactory::createAdd(var_x, const_5);
    entry->addInstruction(add);

    auto* br = LLIRFactory::createBr(merge);
    entry->addInstruction(br);

    // 返回指令
    auto* ret = LLIRFactory::createRet(const_0);
    merge->addInstruction(ret);

    // 建立CFG连接
    entry->addSuccessor(merge);
    then->addSuccessor(merge);
    else_->addSuccessor(merge);
    merge->addPredecessor(entry);
    merge->addPredecessor(then);
    merge->addPredecessor(else_);

    return module;
}

/**
 * @brief 运行演示分析
 */
void runDemoAnalysis() {
    utils::Logger::info("Creating example LLIR module...");
    auto* module = createExampleModule();

    utils::Logger::info("Module dump:");
    std::cout << module->dump() << "\n";

    // 验证模块
    if (!module->validate()) {
        utils::Logger::error("LLIR module validation failed!");
        delete module;
        return;
    }

    utils::Logger::info("LLIR module validation passed");

    // 创建CFG
    auto* func = module->getFunction("test_function");
    if (func) {
        utils::Logger::info("Building CFG for function: " + func->getName());
        CFG cfg(func);

        std::cout << "\nCFG Info:\n";
        std::cout << cfg.toString() << "\n";

        // 导出DOT格式
        std::cout << "\nCFG DOT Format:\n";
        std::cout << cfg.toDOT() << "\n";
    }

    // 创建符号执行配置
    SymbolicExecutionConfig config;
    config.strategy = ExplorationStrategy::DFS;
    config.maxDepth = 10;
    config.maxStates = 100;
    config.verbose = true;

    // 创建符号执行引擎
    utils::Logger::info("Starting symbolic execution...");
    SymbolicExecutionEngine engine(module, config);

    // 运行分析
    engine.runOnFunction("test_function");

    // 打印统计信息
    std::cout << "\n" << engine.getStatistics() << "\n";

    // 清理
    delete module;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    // 解析命令行参数
    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    std::string inputFile;
    bool verbose = false;
    bool runDemo = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return 0;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--demo") {
            runDemo = true;
        } else if (arg[0] != '-') {
            inputFile = arg;
        }
    }

    // 设置日志级别
    if (verbose) {
        utils::Logger::setLevel(utils::Logger::Level::Debug);
    } else {
        utils::Logger::setLevel(utils::Logger::Level::Info);
    }

    utils::Logger::info("CVerifier v" + Version::toString() + " starting...");

    // 运行演示模式
    if (runDemo) {
        utils::Logger::info("Running in demo mode...");
        runDemoAnalysis();
        return 0;
    }

    // TODO: 实现实际的C代码解析和分析
    if (!inputFile.empty()) {
        utils::Logger::warning("Full C parsing not yet implemented");
        utils::Logger::info("Use --demo flag to run the demo analysis");

        utils::Logger::info("Running demo instead...");
        runDemoAnalysis();
    }

    utils::Logger::info("CVerifier completed");

    return 0;
}
