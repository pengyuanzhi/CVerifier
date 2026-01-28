#ifndef CVERIFIER_SYMBOLIC_EXECUTION_ENGINE_H
#define CVERIFIER_SYMBOLIC_EXECUTION_ENGINE_H

#include "cverifier/LLIRModule.h"
#include "cverifier/SymbolicState.h"
#include "cverifier/CFG.h"
#include "cverifier/Core.h"
#include <memory>
#include <queue>
#include <stack>
#include <unordered_set>
#include <vector>

namespace cverifier {
namespace core {

// ============================================================================
// 探索状态
// ============================================================================

/**
 * @brief 探索状态（工作列表项）
 */
struct ExplorationState {
    SymbolicState* symbolicState;  ///< 符号状态
    CFGNode* currentNode;          ///< 当前CFG节点
    int instructionIndex;          ///< 当前指令索引
    std::vector<SourceLocation> trace;  ///< 执行轨迹

    ExplorationState(SymbolicState* state, CFGNode* node)
        : symbolicState(state), currentNode(node), instructionIndex(0) {}
};

// ============================================================================
// 符号执行引擎
// ============================================================================

/**
 * @brief 路径探索策略
 */
enum class ExplorationStrategy {
    DFS,       ///< 深度优先搜索
    BFS,       ///< 广度优先搜索
    Hybrid,    ///< 混合策略（DFS + BFS）
    Random     ///< 随机搜索
};

/**
 * @brief 符号执行配置
 */
struct SymbolicExecutionConfig {
    ExplorationStrategy strategy = ExplorationStrategy::DFS;
    int maxDepth = 100;                    ///< 最大探索深度
    int maxStates = 10000;                 ///< 最大状态数
    int timeout = 300;                     ///< 超时时间（秒）
    bool enableStateMerging = true;        ///< 启用状态合并
    bool enablePathPruning = true;         ///< 启用路径剪枝
    bool verbose = false;                  ///< 详细输出
};

/**
 * @brief 符号执行引擎
 *
 * 核心组件，负责：
 * - 路径探索
 * - 符号状态管理
 * - 指令符号执行
 * - 漏洞检测触发
 */
class SymbolicExecutionEngine {
public:
    SymbolicExecutionEngine(LLIRModule* module, const SymbolicExecutionConfig& config = {});
    ~SymbolicExecutionEngine();

    // 禁止拷贝
    SymbolicExecutionEngine(const SymbolicExecutionEngine&) = delete;
    SymbolicExecutionEngine& operator=(const SymbolicExecutionEngine&) = delete;

    /**
     * @brief 运行符号执行
     */
    void run();

    /**
     * @brief 运行符号执行（指定函数）
     */
    void runOnFunction(const std::string& functionName);

    /**
     * @brief 获取所有可达的符号状态
     */
    const std::vector<SymbolicState*>& getReachedStates() const {
        return reachedStates_;
    }

    /**
     * @brief 获取探索的路径数
     */
    int getExploredPaths() const {
        return exploredPaths_;
    }

    /**
     * @brief 获取发现的漏洞数
     */
    int getFoundVulnerabilities() const {
        return foundVulnerabilities_;
    }

    /**
     * @brief 设置配置
     */
    void setConfig(const SymbolicExecutionConfig& config) {
        config_ = config;
    }

    /**
     * @brief 获取统计信息
     */
    std::string getStatistics() const;

private:
    /**
     * @brief 执行单个基本块
     */
    void executeBasicBlock(
        SymbolicState* state,
        CFGNode* node,
        int startInstIndex
    );

    /**
     * @brief 执行单条指令
     */
    void executeInstruction(
        SymbolicState* state,
        LLIRInstruction* inst,
        CFGNode* node,
        int instIndex
    );

    /**
     * @brief 执行算术运算
     */
    Expr* executeArithmetic(
        SymbolicState* state,
        LLIRInstruction* inst
    );

    /**
     * @brief 执行比较运算
     */
    Expr* executeComparison(
        SymbolicState* state,
        LLIRInstruction* inst
    );

    /**
     * @brief 执行内存操作
     */
    void executeMemory(
        SymbolicState* state,
        LLIRInstruction* inst
    );

    /**
     * @brief 执行分支指令
     */
    void executeBranch(
        SymbolicState* state,
        LLIRInstruction* inst,
        CFGNode* currentNode,
        int instIndex
    );

    /**
     * @brief 执行函数调用
     */
    void executeCall(
        SymbolicState* state,
        LLIRInstruction* inst
    );

    /**
     * @brief 检查漏洞
     */
    void checkVulnerabilities(
        SymbolicState* state,
        LLIRInstruction* inst
    );

    /**
     * @brief 路径探索主循环
     */
    void explore();

    /**
     * @brief DFS探索
     */
    void exploreDFS();

    /**
     * @brief BFS探索
     */
    void exploreBFS();

    /**
     * @brief 混合策略探索
     */
    void exploreHybrid();

    /**
     * @brief 状态合并
     */
    SymbolicState* mergeStates(
        SymbolicState* s1,
        SymbolicState* s2
    );

    /**
     * @brief 路径剪枝检查
     */
    bool shouldPrunePath(SymbolicState* state);

    /**
     * @brief 创建新的变量名
     */
    std::string freshVarName();

    LLIRModule* module_;
    SymbolicExecutionConfig config_;

    std::vector<SymbolicState*> reachedStates_;
    std::queue<ExplorationState*> worklist_;
    std::unordered_set<std::string> visitedStates_;

    int exploredPaths_;
    int foundVulnerabilities_;
    int varCounter_;
    utils::Timer startTimer_;
};

// ============================================================================
// 漏洞检测器接口
// ============================================================================

/**
 * @brief 漏洞检测器基类
 */
class VulnerabilityChecker {
public:
    virtual ~VulnerabilityChecker() = default;

    /**
     * @brief 检查当前状态是否存在漏洞
     * @return 如果发现漏洞，返回漏洞报告；否则返回nullptr
     */
    virtual VulnerabilityReport* check(
        SymbolicState* state,
        LLIRInstruction* inst
    ) = 0;

    /**
     * @brief 获取检测器名称
     */
    virtual std::string getName() const = 0;
};

// ============================================================================
// 缓冲区溢出检测器
// ============================================================================

/**
 * @brief 缓冲区溢出检测器
 */
class BufferOverflowChecker : public VulnerabilityChecker {
public:
    VulnerabilityReport* check(
        SymbolicState* state,
        LLIRInstruction* inst
    ) override;

    std::string getName() const override {
        return "BufferOverflow";
    }

private:
    bool isSafeAccess(
        Expr* ptr,
        Expr* buffer,
        Expr* size,
        SymbolicState* state
    );
};

// ============================================================================
// 空指针解引用检测器
// ============================================================================

/**
 * @brief 空指针解引用检测器
 */
class NullPointerChecker : public VulnerabilityChecker {
public:
    VulnerabilityReport* check(
        SymbolicState* state,
        LLIRInstruction* inst
    ) override;

    std::string getName() const override {
        return "NullPointerDereference";
    }
};

// ============================================================================
// 内存泄漏检测器
// ============================================================================

/**
 * @brief 内存泄漏检测器
 */
class MemoryLeakChecker : public VulnerabilityChecker {
public:
    VulnerabilityReport* check(
        SymbolicState* state,
        LLIRInstruction* inst
    ) override;

    std::string getName() const override {
        return "MemoryLeak";
    }
};

// ============================================================================
// 整数溢出检测器
// ============================================================================

/**
 * @brief 整数溢出检测器
 */
class IntegerOverflowChecker : public VulnerabilityChecker {
public:
    VulnerabilityReport* check(
        SymbolicState* state,
        LLIRInstruction* inst
    ) override;

    std::string getName() const override {
        return "IntegerOverflow";
    }
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_SYMBOLIC_EXECUTION_ENGINE_H
