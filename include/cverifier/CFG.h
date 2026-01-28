#ifndef CVERIFIER_CFG_H
#define CVERIFIER_CFG_H

#include "cverifier/LLIRModule.h"
#include "cverifier/Core.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cverifier {
namespace core {

// ============================================================================
// 控制流图 (CFG)
// ============================================================================

/**
 * @brief CFG节点（基本块）
 */
class CFGNode {
public:
    explicit CFGNode(LLIRBasicBlock* bb)
        : basicBlock_(bb) {}

    LLIRBasicBlock* getBasicBlock() const { return basicBlock_; }

    const std::vector<CFGNode*>& getSuccessors() const {
        return successors_;
    }

    const std::vector<CFGNode*>& getPredecessors() const {
        return predecessors_;
    }

    void addSuccessor(CFGNode* node) {
        successors_.push_back(node);
    }

    void addPredecessor(CFGNode* node) {
        predecessors_.push_back(node);
    }

    std::string getId() const {
        return basicBlock_->getName();
    }

private:
    LLIRBasicBlock* basicBlock_;
    std::vector<CFGNode*> successors_;
    std::vector<CFGNode*> predecessors_;
};

/**
 * @brief 控制流图
 *
 * 表示函数的控制流结构，用于符号执行的路径探索
 */
class CFG {
public:
    explicit CFG(LLIRFunction* function);
    ~CFG();

    // 禁止拷贝
    CFG(const CFG&) = delete;
    CFG& operator=(const CFG&) = delete;

    /**
     * @brief 获取入口节点
     */
    CFGNode* getEntryNode() const { return entryNode_; }

    /**
     * @brief 获取出口节点集合
     */
    const std::vector<CFGNode*>& getExitNodes() const {
        return exitNodes_;
    }

    /**
     * @brief 获取所有节点
     */
    const std::unordered_map<std::string, CFGNode*>& getNodes() const {
        return nodes_;
    }

    /**
     * @brief 根据基本块名称查找节点
     */
    CFGNode* getNode(const std::string& name) const {
        auto it = nodes_.find(name);
        return it != nodes_.end() ? it->second : nullptr;
    }

    /**
     * @brief 获取相关函数
     */
    LLIRFunction* getFunction() const { return function_; }

    /**
     * @brief 计算支配关系
     */
    void computeDominators();

    /**
     * @brief 计算后支配关系
     */
    void computePostDominators();

    /**
     * @brief 检查节点是否支配另一个节点
     */
    bool dominates(CFGNode* a, CFGNode* b) const;

    /**
     * @brief 检查节点是否后支配另一个节点
     */
    bool postDominates(CFGNode* a, CFGNode* b) const;

    /**
     * @brief 获取节点的支配边界
     */
    std::vector<CFGNode*> getDominanceFrontier(CFGNode* node) const;

    /**
     * @brief 找到所有回边（用于循环检测）
     */
    std::vector<std::pair<CFGNode*, CFGNode*>> findBackEdges() const;

    /**
     * @brief 检查是否存在路径从from到to
     */
    bool hasPath(CFGNode* from, CFGNode* to) const;

    /**
     * @brief 计算节点的深度（从入口开始）
     */
    int computeDepth(CFGNode* node) const;

    /**
     * @brief 获取所有循环（自然循环）
     */
    std::vector<std::vector<CFGNode*>> findLoops() const;

    /**
     * @brief 转换为字符串（用于调试）
     */
    std::string toString() const;

    /**
     * @brief 导出为DOT格式（用于可视化）
     */
    std::string toDOT() const;

private:
    /**
     * @brief 构建CFG
     */
    void build();

    /**
     * @brief 识别入口和出口节点
     */
    void identifyEntryAndExit();

    LLIRFunction* function_;
    CFGNode* entryNode_;
    std::vector<CFGNode*> exitNodes_;
    std::unordered_map<std::string, CFGNode*> nodes_;

    // 支配关系缓存
    std::unordered_map<CFGNode*, std::unordered_set<CFGNode*>> dominators_;
    std::unordered_map<CFGNode*, std::unordered_set<CFGNode*>> postDominators_;
    std::unordered_map<CFGNode*, std::vector<CFGNode*>> dominanceFrontier_;
};

// ============================================================================
// CFG遍历器
// ============================================================================

/**
 * @brief CFG遍历顺序
 */
enum class TraversalOrder {
    PreOrder,    ///< 前序遍历
    PostOrder,   ///< 后序遍历
    ReversePostOrder,  ///< 逆后序遍历
    BFS          ///< 广度优先遍历
};

/**
 * @brief CFG遍历器
 *
 * 提供多种遍历策略用于符号执行的路径探索
 */
class CFGTraversal {
public:
    explicit CFGTraversal(CFG* cfg)
        : cfg_(cfg) {}

    /**
     * @brief 前序遍历
     */
    std::vector<CFGNode*> preOrderTraversal();

    /**
     * @brief 后序遍历
     */
    std::vector<CFGNode*> postOrderTraversal();

    /**
     * @brief 逆后序遍历（常用于数据流分析）
     */
    std::vector<CFGNode*> reversePostOrderTraversal();

    /**
     * @brief 广度优先遍历
     */
    std::vector<CFGNode*> bfsTraversal();

    /**
     * @brief 深度优先遍历
     */
    std::vector<CFGNode*> dfsTraversal();

private:
    void preOrderDFS(CFGNode* node, std::unordered_set<CFGNode*>& visited, std::vector<CFGNode*>& result);
    void postOrderDFS(CFGNode* node, std::unordered_set<CFGNode*>& visited, std::vector<CFGNode*>& result);

    CFG* cfg_;
};

// ============================================================================
// 路径收集器
// ============================================================================

/**
 * @brief 执行路径
 */
class ExecutionPath {
public:
    ExecutionPath() = default;

    void addNode(CFGNode* node) {
        nodes_.push_back(node);
    }

    const std::vector<CFGNode*>& getNodes() const {
        return nodes_;
    }

    std::string toString() const {
        std::string result = "Path: ";
        for (size_t i = 0; i < nodes_.size(); ++i) {
            result += nodes_[i]->getId();
            if (i < nodes_.size() - 1) {
                result += " -> ";
            }
        }
        return result;
    }

private:
    std::vector<CFGNode*> nodes_;
};

/**
 * @brief 路径收集器
 *
 * 收集CFG中的所有执行路径（注意：可能指数级增长）
 */
class PathCollector {
public:
    explicit PathCollector(CFG* cfg)
        : cfg_(cfg) {}

    /**
     * @brief 收集所有路径
     */
    std::vector<ExecutionPath> collectAllPaths();

    /**
     * @brief 收集所有路径（带深度限制）
     */
    std::vector<ExecutionPath> collectAllPathsWithLimit(int maxDepth);

    /**
     * @brief 收集从入口到特定节点的路径
     */
    std::vector<ExecutionPath> collectPathsTo(CFGNode* target);

private:
    void dfsCollectPaths(
        CFGNode* current,
        std::vector<CFGNode*>& currentPath,
        std::unordered_set<CFGNode*>& visited,
        std::vector<ExecutionPath>& paths,
        int depth,
        int maxDepth
    );

    CFG* cfg_;
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_CFG_H
