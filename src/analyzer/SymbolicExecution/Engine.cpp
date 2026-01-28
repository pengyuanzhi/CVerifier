/**
 * @file Engine.cpp
 * @brief 符号执行引擎实现
 */

#include "cverifier/SymbolicExecutionEngine.h"
#include "cverifier/Utils.h"
#include <sstream>
#include <algorithm>
#include <random>

namespace cverifier {
namespace core {

// ============================================================================
// SymbolicExecutionEngine 实现
// ============================================================================

SymbolicExecutionEngine::SymbolicExecutionEngine(
    LLIRModule* module,
    const SymbolicExecutionConfig& config
) : module_(module),
    config_(config),
    exploredPaths_(0),
    foundVulnerabilities_(0),
    varCounter_(0) {
    startTime_ = utils::Timer().elapsedSec();
}

SymbolicExecutionEngine::~SymbolicExecutionEngine() {
    // 清理工作列表
    while (!worklist_.empty()) {
        ExplorationState* explState = worklist_.front();
        worklist_.pop();
        if (explState) {
            if (explState->symbolicState) {
                delete explState->symbolicState;
            }
            delete explState;
        }
    }

    // 清理可达状态
    for (auto* state : reachedStates_) {
        if (state) {
            delete state;
        }
    }
}

void SymbolicExecutionEngine::run() {
    // 获取模块中的所有函数
    auto& functions = module_->getFunctions();

    for (auto* func : functions) {
        runOnFunction(func->getName());
    }
}

void SymbolicExecutionEngine::runOnFunction(const std::string& functionName) {
    auto* func = module_->getFunction(functionName);
    if (!func) {
        utils::Logger::warning("Function not found: " + functionName);
        return;
    }

    utils::Logger::info("Starting symbolic execution for function: " + functionName);

    // 创建CFG
    utils::Logger::debug("Creating CFG for function: " + functionName);
    auto* cfg = new CFG(func);

    // 创建初始状态
    utils::Logger::debug("Creating initial symbolic state");
    auto* initialState = new SymbolicState(nullptr);

    utils::Logger::debug("Getting entry node from CFG");
    // 获取入口节点
    CFGNode* entryNode = cfg->getEntryNode();
    if (!entryNode) {
        utils::Logger::error("No entry node found for function: " + functionName);
        delete cfg;
        delete initialState;
        return;
    }

    utils::Logger::debug("Entry node found: " + entryNode->getId());
    utils::Logger::debug("Creating initial exploration state");

    // 创建初始探索状态并加入工作列表
    auto* initialExplorationState = new ExplorationState(initialState, entryNode);

    utils::Logger::debug("Adding exploration state to worklist");
    worklist_.push(initialExplorationState);

    utils::Logger::debug("Worklist size after push: " + std::to_string(worklist_.size()));

    // 开始探索
    explore();

    // 清理
    delete cfg;

    utils::Logger::info("Symbolic execution completed for function: " + functionName);
}

void SymbolicExecutionEngine::explore() {
    utils::Logger::info("Starting path exploration with " +
                       std::to_string(worklist_.size()) + " initial states");

    utils::Logger::debug("About to enter exploration loop");

    int iterations = 0;
    while (!worklist_.empty()) {
        ++iterations;

        utils::Logger::debug("Iteration " + std::to_string(iterations) +
                           ", worklist size: " + std::to_string(worklist_.size()));

        utils::Logger::debug("Fetching exploration state from worklist");

        // 从工作列表中取出一个状态
        ExplorationState* explorationState = worklist_.front();

        utils::Logger::debug("Got exploration state, popping from worklist");
        worklist_.pop();

        utils::Logger::debug("Extracting state and node from exploration state");

        SymbolicState* state = explorationState->symbolicState;
        CFGNode* node = explorationState->currentNode;

        if (!state || !node) {
            utils::Logger::error("Null state or node in exploration state!");
            if (explorationState) {
                delete explorationState;
            }
            continue;
        }

        utils::Logger::debug("Processing node: " + node->getId());

        // 路径剪枝检查
        if (config_.enablePathPruning && shouldPrunePath(state)) {
            utils::Logger::debug("Path pruned, skipping state");
            delete state;
            delete explorationState;
            continue;
        }

        // 检查超时
        double elapsed = utils::Timer().elapsedSec() - startTime_;
        if (elapsed > config_.timeout) {
            utils::Logger::warning("Symbolic execution timeout");
            delete state;
            delete explorationState;
            break;
        }

        // 检查状态数限制
        if (reachedStates_.size() >= static_cast<size_t>(config_.maxStates)) {
            utils::Logger::warning("Maximum number of states reached");
            break;
        }

        // 从工作列表中取出一个状态
        ExplorationState* explorationState = worklist_.front();
        worklist_.pop();

        SymbolicState* state = explorationState->symbolicState;
        CFGNode* node = explorationState->currentNode;

        utils::Logger::debug("Processing node: " + node->getId());

        // 路径剪枝检查
        if (config_.enablePathPruning && shouldPrunePath(state)) {
            utils::Logger::debug("Path pruned, skipping state");
            delete state;
            delete explorationState;
            continue;
        }

        // 执行基本块
        executeBasicBlock(
            state,
            node,
            explorationState->instructionIndex
        );

        // 将状态加入可达状态集合（现在状态的所有权转移到 reachedStates_）
        reachedStates_.push_back(state);

        // 删除探索状态包装器，但不删除 symbolicState
        explorationState->symbolicState = nullptr;
        delete explorationState;
    }

    utils::Logger::info("Explored " + std::to_string(exploredPaths_) + " paths");
}

void SymbolicExecutionEngine::executeBasicBlock(
    SymbolicState* state,
    CFGNode* node,
    int startInstIndex
) {
    LLIRBasicBlock* bb = node->getBasicBlock();
    const auto& instructions = bb->getInstructions();

    // 从指定索引开始执行指令
    for (size_t i = startInstIndex; i < instructions.size(); ++i) {
        LLIRInstruction* inst = instructions[i];

        // 执行指令
        executeInstruction(state, inst, node, static_cast<int>(i));

        // 检查漏洞
        checkVulnerabilities(state, inst);
    }
}

void SymbolicExecutionEngine::executeInstruction(
    SymbolicState* state,
    LLIRInstruction* inst,
    CFGNode* node,
    int instIndex
) {
    switch (inst->getType()) {
        case LLIRInstructionType::Add:
        case LLIRInstructionType::Sub:
        case LLIRInstructionType::Mul:
        case LLIRInstructionType::Div:
        case LLIRInstructionType::Rem: {
            // 算术运算：生成新的符号变量
            std::string varName = freshVarName();
            auto* result = new VariableExpr(varName);
            // TODO: 构建完整的符号表达式
            break;
        }

        case LLIRInstructionType::ICmp:
        case LLIRInstructionType::FCmp: {
            // 比较运算
            executeComparison(state, inst);
            break;
        }

        case LLIRInstructionType::Load:
        case LLIRInstructionType::Store:
        case LLIRInstructionType::GetElementPtr: {
            // 内存操作
            executeMemory(state, inst);
            break;
        }

        case LLIRInstructionType::Br: {
            // 分支指令
            executeBranch(state, inst, node, instIndex);
            break;
        }

        case LLIRInstructionType::Ret: {
            // 返回指令：路径结束
            exploredPaths_++;
            break;
        }

        case LLIRInstructionType::Call: {
            // 函数调用
            executeCall(state, inst);
            break;
        }

        case LLIRInstructionType::Alloca: {
            // 栈上分配：创建新的符号变量
            std::string varName = freshVarName();
            auto* var = new VariableExpr(varName);
            state->assign(varName, var);
            break;
        }

        default:
            // 其他指令暂时忽略
            break;
    }
}

Expr* SymbolicExecutionEngine::executeArithmetic(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // 简化实现：总是返回一个新的符号变量
    std::string varName = freshVarName();
    return new VariableExpr(varName);
}

Expr* SymbolicExecutionEngine::executeComparison(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // 简化实现：总是返回一个新的布尔变量
    std::string varName = freshVarName() + "_cmp";
    return new VariableExpr(varName);
}

void SymbolicExecutionEngine::executeMemory(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // 简化实现：内存操作暂不详细处理
    // TODO: 实现完整的内存模型
}

void SymbolicExecutionEngine::executeBranch(
    SymbolicState* state,
    LLIRInstruction* inst,
    CFGNode* currentNode,
    int instIndex
) {
    // 简化实现：无条件分支，直接跳转到后继节点
    const auto& successors = currentNode->getSuccessors();

    if (successors.empty()) {
        utils::Logger::warning("Branch instruction has no successors");
        return;
    }

    for (auto* succ : successors) {
        // 克隆状态
        auto cloned = state->clone();
        auto* newState = cloned.release();

        if (!newState) {
            utils::Logger::error("Failed to clone symbolic state");
            continue;
        }

        // 创建新的探索状态
        auto* newExplorationState = new ExplorationState(newState, succ);
        newExplorationState->instructionIndex = 0;

        // 加入工作列表
        worklist_.push(newExplorationState);

        utils::Logger::debug("Added new exploration state for node: " + succ->getId());
    }
}

void SymbolicExecutionEngine::executeCall(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // 简化实现：函数调用暂不处理
    // TODO: 实现函数调用和返回
}

void SymbolicExecutionEngine::checkVulnerabilities(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // TODO: 集成各个漏洞检测器
    // 这里简化实现，仅作演示

    // 检查是否有load指令（可能的空指针解引用）
    if (inst->getType() == LLIRInstructionType::Load) {
        // 创建检测器
        NullPointerChecker checker;
        auto* report = checker.check(state, inst);
        if (report) {
            foundVulnerabilities_++;
            utils::Logger::error("Vulnerability found: " + report->toString());
            delete report;
        }
    }

    // 检查是否有store指令（可能的缓冲区溢出）
    if (inst->getType() == LLIRInstructionType::Store) {
        BufferOverflowChecker checker;
        auto* report = checker.check(state, inst);
        if (report) {
            foundVulnerabilities_++;
            utils::Logger::error("Vulnerability found: " + report->toString());
            delete report;
        }
    }
}

SymbolicState* SymbolicExecutionEngine::mergeStates(
    SymbolicState* s1,
    SymbolicState* s2
) {
    // 简化的状态合并：返回s1的克隆
    // 实际实现中需要更复杂的合并算法
    return s1->clone().release();
}

bool SymbolicExecutionEngine::shouldPrunePath(SymbolicState* state) {
    // 简化实现：检查路径约束是否可满足
    // 实际实现中需要调用SMT求解器
    return !state->getPathConstraint()->isSatisfiable();
}

std::string SymbolicExecutionEngine::freshVarName() {
    return "v" + std::to_string(varCounter_++);
}

std::string SymbolicExecutionEngine::getStatistics() const {
    std::ostringstream oss;

    oss << "Symbolic Execution Statistics:\n";
    oss << "  Explored Paths: " << exploredPaths_ << "\n";
    oss << "  Reached States: " << reachedStates_.size() << "\n";
    oss << "  Found Vulnerabilities: " << foundVulnerabilities_ << "\n";

    double elapsed = utils::Timer().elapsedSec() - startTime_;
    oss << "  Elapsed Time: " << std::fixed << elapsed << "s\n";

    return oss.str();
}

// ============================================================================
// BufferOverflowChecker 实现
// ============================================================================

VulnerabilityReport* BufferOverflowChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    if (inst->getType() != LLIRInstructionType::Store) {
        return nullptr;
    }

    // 简化实现：检测所有store指令为潜在溢出
    // 实际实现中需要分析地址和大小

    auto* report = new VulnerabilityReport();
    report->type = VulnerabilityType::BufferOverflow;
    report->severity = Severity::High;
    report->location = inst->getLocation();
    report->message = "Potential buffer overflow detected";
    report->description = "Store operation may write beyond buffer bounds";

    return report;
}

bool BufferOverflowChecker::isSafeAccess(
    Expr* ptr,
    Expr* buffer,
    Expr* size,
    SymbolicState* state
) {
    // 简化实现：总是返回true（假设安全）
    // 实际实现中需要使用SMT求解器验证
    return true;
}

// ============================================================================
// NullPointerChecker 实现
// ============================================================================

VulnerabilityReport* NullPointerChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    if (inst->getType() != LLIRInstructionType::Load) {
        return nullptr;
    }

    // 简化实现：检测所有load指令为潜在空指针解引用
    // 实际实现中需要分析指针是否可能为NULL

    auto* report = new VulnerabilityReport();
    report->type = VulnerabilityType::NullPointerDereference;
    report->severity = Severity::Critical;
    report->location = inst->getLocation();
    report->message = "Potential null pointer dereference detected";
    report->description = "Load operation may dereference a null pointer";

    return report;
}

// ============================================================================
// MemoryLeakChecker 实现
// ============================================================================

VulnerabilityReport* MemoryLeakChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // 简化实现：检测函数返回时是否有未释放的内存
    if (inst->getType() != LLIRInstructionType::Ret) {
        return nullptr;
    }

    auto* heap = state->getHeap();
    auto unfreedObjects = heap->getUnfreedObjects();

    if (!unfreedObjects.empty()) {
        auto* report = new VulnerabilityReport();
        report->type = VulnerabilityType::MemoryLeak;
        report->severity = Severity::Medium;
        report->location = inst->getLocation();
        report->message = "Memory leak detected";
        report->description = std::to_string(unfreedObjects.size()) +
                            " allocated object(s) not freed";
        return report;
    }

    return nullptr;
}

// ============================================================================
// IntegerOverflowChecker 实现
// ============================================================================

VulnerabilityReport* IntegerOverflowChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    // 简化实现：检测所有算术运算为潜在溢出
    auto type = inst->getType();

    if (type != LLIRInstructionType::Add &&
        type != LLIRInstructionType::Sub &&
        type != LLIRInstructionType::Mul) {
        return nullptr;
    }

    auto* report = new VulnerabilityReport();
    report->type = VulnerabilityType::IntegerOverflow;
    report->severity = Severity::High;
    report->location = inst->getLocation();
    report->message = "Potential integer overflow detected";
    report->description = "Arithmetic operation may cause integer overflow";

    return report;
}

} // namespace core
} // namespace cverifier
