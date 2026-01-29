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

#ifdef HAVE_LLVM
#include "cverifier/ClangParser.h"
#endif

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
    std::cout << "=============================================================================\n";
    std::cout << "基本选项:\n";
    std::cout << "=============================================================================\n";
    std::cout << "  --help, -h              显示此帮助信息\n";
    std::cout << "  --version, -v           显示版本信息\n";
    std::cout << "  --verbose, -V           详细输出（多级可重复：-VV, -VVV）\n";
    std::cout << "  --config <文件>         指定配置文件（YAML格式）\n";
    std::cout << "  --demo                  运行演示模式（不分析实际文件）\n";
    std::cout << "\n";
    std::cout << "=============================================================================\n";
    std::cout << "分析选项:\n";
    std::cout << "=============================================================================\n";
    std::cout << "  --entry <函数名>        指定入口函数（默认：main）\n";
    std::cout << "  --timeout <秒>          设置超时时间（默认：300秒）\n";
    std::cout << "  --max-depth <深度>      设置最大探索深度（默认：100）\n";
    std::cout << "  --max-states <数量>     设置最大状态数（默认：10000）\n";
    std::cout << "  --strategy <策略>       路径探索策略：dfs, bfs, hybrid（默认：hybrid）\n";
    std::cout << "  --enable-abstract       启用抽象解释加速分析\n";
    std::cout << "  --domain <域>           抽象域类型：constant, interval（默认：interval）\n";
    std::cout << "  --threads <数量>        并行分析线程数（默认：4，0表示禁用）\n";
    std::cout << "\n";
    std::cout << "=============================================================================\n";
    std::cout << "漏洞检测器:\n";
    std::cout << "=============================================================================\n";
    std::cout << "  --enable-all            启用所有检查器\n";
    std::cout << "  --disable-all           禁用所有检查器\n";
    std::cout << "\n";
    std::cout << "内存安全:\n";
    std::cout << "  --check-buffer          缓冲区溢出检测（栈/堆）\n";
    std::cout << "  --check-null            空指针解引用检测\n";
    std::cout << "  --check-leak            内存泄漏检测\n";
    std::cout << "  --check-use-after-free  Use-after-free检测\n";
    std::cout << "  --check-double-free     Double-free检测\n";
    std::cout << "\n";
    std::cout << "算术安全:\n";
    std::cout << "  --check-overflow        整数溢出检测\n";
    std::cout << "  --check-float-overflow  浮点溢出检测\n";
    std::cout << "  --check-div-by-zero     除零检测\n";
    std::cout << "\n";
    std::cout << "代码质量:\n";
    std::cout << "  --check-uninit          未初始化变量检测\n";
    std::cout << "  --check-dead-code       死代码检测\n";
    std::cout << "\n";
    std::cout << "=============================================================================\n";
    std::cout << "报告选项:\n";
    std::cout << "=============================================================================\n";
    std::cout << "  --output <文件>         输出报告到文件\n";
    std::cout << "  --format <格式>         报告格式：console, json, sarif（默认：console）\n";
    std::cout << "  --severity <级别>       最小报告级别：low, medium, high, critical（默认：low）\n";
    std::cout << "  --no-trace              不包含错误轨迹\n";
    std::cout << "  --no-suggestions        不包含修复建议\n";
    std::cout << "  --stats-only            仅输出统计信息\n";
    std::cout << "\n";
    std::cout << "=============================================================================\n";
    std::cout << "示例:\n";
    std::cout << "=============================================================================\n";
    std::cout << "  # 基本分析\n";
    std::cout << "  " << programName << " test.c\n";
    std::cout << "\n";
    std::cout << "  # 启用所有检查器，详细输出\n";
    std::cout << "  " << programName << " --verbose --enable-all src/*.c\n";
    std::cout << "\n";
    std::cout << "  # 使用配置文件\n";
    std::cout << "  " << programName << " --config=aggressive.yaml main.c\n";
    std::cout << "\n";
    std::cout << "  # 仅检测缓冲区溢出和空指针\n";
    std::cout << "  " << programName << " --check-buffer --check-null test.c\n";
    std::cout << "\n";
    std::cout << "  # 生成SARIF格式报告\n";
    std::cout << "  " << programName << " --enable-all --format sarif --output report.sarif test.c\n";
    std::cout << "\n";
    std::cout << "  # 混合分析：抽象解释 + 符号执行\n";
    std::cout << "  " << programName << " --enable-abstract --strategy hybrid --max-depth 50 test.c\n";
    std::cout << "\n";
    std::cout << "  # 运行演示模式\n";
    std::cout << "  " << programName << " --demo\n";
    std::cout << "\n";
}

/**
 * @brief 打印版本信息
 */
void printVersion() {
    std::cout << "CVerifier v" << Version::toString() << "\n";
    std::cout << "C++ 形式验证工具 - 符号执行 + 抽象解释\n";
    std::cout << "\n";
    std::cout << "核心特性:\n";
    std::cout << "  - 符号执行引擎（路径敏感分析）\n";
    std::cout << "  - 抽象解释器（区间域/常量域）\n";
    std::cout << "  - 混合分析策略（快速剪枝 + 精确验证）\n";
    std::cout << "  - Z3 SMT求解器集成\n";
    std::cout << "  - Clang前端（支持完整C11/C17）\n";
    std::cout << "\n";
    std::cout << "漏洞检测能力:\n";
    std::cout << "\n";
    std::cout << "内存安全:\n";
    std::cout << "  - 缓冲区溢出（栈/堆）\n";
    std::cout << "  - 空指针解引用\n";
    std::cout << "  - 内存泄漏\n";
    std::cout << "  - Use-after-free\n";
    std::cout << "  - Double-free\n";
    std::cout << "\n";
    std::cout << "算术安全:\n";
    std::cout << "  - 整数溢出\n";
    std::cout << "  - 浮点溢出\n";
    std::cout << "  - 除零错误\n";
    std::cout << "\n";
    std::cout << "代码质量:\n";
    std::cout << "  - 未初始化变量\n";
    std::cout << "  - 死代码检测\n";
    std::cout << "\n";
    std::cout << "输出格式:\n";
    std::cout << "  - 控制台输出（人类可读）\n";
    std::cout << "  - JSON格式（机器可读）\n";
    std::cout << "  - SARIF格式（IDE集成）\n";
    std::cout << "\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
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
 * @brief 分析 C 源文件
 */
void analyzeCFile(const std::string& filename) {
    utils::Logger::info("Analyzing C file: " + filename);

#ifdef HAVE_LLVM
    // 创建 Clang 解析器
    frontend::ClangParser parser;

    // 解析文件
    LLIRModule* module = parser.parseFile(filename);

    if (!module) {
        utils::Logger::error("Failed to parse file: " + parser.getLastError());
        return;
    }

    utils::Logger::info("File parsed successfully!");
    std::cout << "\nModule: " << module->getName() << "\n";
    std::cout << "Functions: " << module->getFunctions().size() << "\n";

    // 分析每个函数
    for (auto* func : module->getFunctions()) {
        std::cout << "\n============================================================\n";
        std::cout << "Function: " << func->getName() << "\n";
        std::cout << "============================================================\n";

        // 创建CFG
        CFG cfg(func);
        std::cout << "CFG Nodes: " << cfg.getNodes().size() << "\n";

        // 创建符号执行配置
        SymbolicExecutionConfig config;
        config.maxDepth = 100;
        config.maxStates = 1000;
        config.timeout = 60;
        config.verbose = utils::Logger::getLevel() >= utils::Logger::Level::Debug;

        // 运行符号执行
        std::cout << "\nRunning symbolic execution...\n";
        SymbolicExecutionEngine engine(module, config);
        engine.runOnFunction(func->getName());

        // 打印统计信息
        std::cout << "\n" << engine.getStatistics() << "\n";

        // 打印发现的漏洞
        int vulns = engine.getFoundVulnerabilities();
        if (vulns > 0) {
            std::cout << "⚠️  Found " << vulns << " potential vulnerabilit"
                      << (vulns > 1 ? "ies" : "y") << "!\n";
        } else {
            std::cout << "✅ No vulnerabilities detected\n";
        }
    }

    // 清理
    delete module;

    utils::Logger::info("Analysis completed");
#else
    utils::Logger::error("LLVM/Clang not available. Cannot parse C files.");
    utils::Logger::info("Please install LLVM/Clang 15+ to enable C file analysis.");
#endif
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
        // Demo 模式自动启用详细日志
        utils::Logger::setLevel(utils::Logger::Level::Debug);
        utils::Logger::info("Debug logging enabled for demo mode");
        runDemoAnalysis();
        return 0;
    }

    // 分析C源文件
    if (!inputFile.empty()) {
        // 检查文件扩展名
        if (inputFile.size() >= 2 && inputFile.substr(inputFile.size() - 2) == ".c") {
            // C源文件 - 使用Clang前端
            analyzeCFile(inputFile);
        } else {
            utils::Logger::warning("Unsupported file type");
            utils::Logger::info("Currently only .c files are supported");
            utils::Logger::info("Use --demo flag to run the demo analysis");
            runDemoAnalysis();
        }
    }

    utils::Logger::info("CVerifier completed");

    return 0;
}
