/**
 * CustomChecker.cpp - 示例插件实现
 */

#include "CustomChecker.h"
#include "cverifier/Core.h"
#include <iostream>

namespace cverifier {

CustomChecker::CustomChecker()
    : initialized_(false) {
}

CustomChecker::~CustomChecker() {
    shutdown();
}

std::string CustomChecker::getName() const {
    return "CustomChecker";
}

std::string CustomChecker::getVersion() const {
    return "1.0.0";
}

std::string CustomChecker::getDescription() const {
    return "Example plugin that demonstrates custom vulnerability checking:\n"
           "- Uninitialized variable detection\n"
           "- Dead code detection\n"
           "- Float accumulation error detection";
}

void CustomChecker::initialize(const PluginConfig& config) {
    std::cout << "CustomChecker::initialize() called\n";
    std::cout << "  Config name: " << config.name << "\n";
    std::cout << "  Config enabled: " << (config.enabled ? "true" : "false") << "\n";
    std::cout << "  Config priority: " << config.priority << "\n";

    config_ = config;
    initialized_ = true;
}

bool CustomChecker::verify(LLIRFunction* function) {
    if (!initialized_) {
        std::cerr << "CustomChecker not initialized!\n";
        return false;
    }

    std::cout << "CustomChecker::verify() called for function: "
              << function->getName() << "\n";

    reports_.clear();

    // 执行各种检查
    checkUninitializedVariables(function);
    checkDeadCode(function);
    checkFloatAccumulation(function);

    // 如果发现任何漏洞，返回 false
    return reports_.empty();
}

std::vector<VulnerabilityReport> CustomChecker::getReports() const {
    return reports_;
}

void CustomChecker::shutdown() {
    if (initialized_) {
        std::cout << "CustomChecker::shutdown() called\n";
        reports_.clear();
        initialized_ = false;
    }
}

bool CustomChecker::isHealthy() const {
    return initialized_;
}

void CustomChecker::checkUninitializedVariables(LLIRFunction* function) {
    std::cout << "  Checking for uninitialized variables...\n";

    // 示例：遍历所有指令，查找未初始化的变量使用
    for (auto& bb : function->getBasicBlocks()) {
        for (auto& inst : bb->getInstructions()) {
            // 这里是简化示例
            // 实际实现需要：
            // 1. 构建def-use链
            // 2. 跟踪变量初始化状态
            // 3. 检测未初始化的读取

            // 示例：检测可能的未初始化使用
            if (inst->getOpcode() == Instruction::Load) {
                auto* loadInst = static_cast<LoadInst*>(inst.get());

                // 检查操作数是否已初始化
                // 这里需要与符号执行引擎集成
                bool isInitialized = false;  // 简化示例

                if (!isInitialized) {
                    auto report = createReport(
                        inst->getLocation(),
                        "uninitialized-variable",
                        "Potential use of uninitialized variable",
                        Severity::Warning
                    );
                    reports_.push_back(report);
                }
            }
        }
    }
}

void CustomChecker::checkDeadCode(LLIRFunction* function) {
    std::cout << "  Checking for dead code...\n";

    // 示例：检测不可达的基本块
    auto& entryBlock = function->getEntryBlock();

    // 构建控制流图并分析可达性
    std::set<LLIRBasicBlock*> reachable;
    // TODO: 实现完整的可达性分析

    // 简化示例：查找没有前驱的基本块（除了入口块）
    for (auto& bb : function->getBasicBlocks()) {
        if (bb.get() == &entryBlock) {
            continue;  // 跳过入口块
        }

        if (bb->getPredecessors().empty()) {
            auto report = createReport(
                bb->getFirstNonDebugLocation(),
                "dead-code",
                "Unreachable code detected (no predecessors)",
                Severity::Info
            );
            reports_.push_back(report);
        }
    }
}

void CustomChecker::checkFloatAccumulation(LLIRFunction* function) {
    std::cout << "  Checking for float accumulation errors...\n";

    // 示例：检测循环中的浮点累加
    for (auto& bb : function->getBasicBlocks()) {
        if (!bb->isLoopHeader()) {
            continue;
        }

        // 检测循环中的浮点加法/减法
        for (auto& inst : bb->getInstructions()) {
            if (inst->getOpcode() == Instruction::FAdd ||
                inst->getOpcode() == Instruction::FSub) {

                auto report = createReport(
                    inst->getLocation(),
                    "float-accumulation",
                    "Potential float accumulation error in loop",
                    Severity::Warning
                );
                report.trace.push_back({
                    inst->getLocation(),
                    "Consider using Kahan summation or compensated summation"
                });
                reports_.push_back(report);
            }
        }
    }
}

VulnerabilityReport CustomChecker::createReport(
    const SourceLocation& loc,
    const std::string& type,
    const std::string& message,
    Severity severity
) {
    VulnerabilityReport report;
    report.type = type;
    report.severity = severity;
    report.location = loc;
    report.message = message;

    return report;
}

} // namespace cverifier

// ============================================================================
// 插件工厂函数（必须导出）
// ============================================================================

extern "C" {

/**
 * 创建插件实例
 * CVerifier 加载插件时会调用此函数
 */
cverifier::IPlugin* createPlugin() {
    return new cverifier::CustomChecker();
}

/**
 * 销毁插件实例
 * CVerifier 卸载插件时会调用此函数
 */
void destroyPlugin(cverifier::IPlugin* plugin) {
    delete plugin;
}

} // extern "C"
