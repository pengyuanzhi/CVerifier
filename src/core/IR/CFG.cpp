/**
 * @file CFG.cpp
 * @brief 控制流图实现
 */

#include "cverifier/CFG.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <sstream>

namespace cverifier {
namespace core {

// ============================================================================
// CFG 实现
// ============================================================================

CFG::CFG(LLIRFunction* function)
    : function_(function), entryNode_(nullptr) {
    build();
    identifyEntryAndExit();
}

CFG::~CFG() {
    for (auto& [name, node] : nodes_) {
        delete node;
    }
}

void CFG::build() {
    // 为每个基本块创建CFG节点
    for (auto* bb : function_->getBasicBlocks()) {
        auto* node = new CFGNode(bb);
        nodes_[bb->getName()] = node;
    }

    // 建立边连接
    for (auto* bb : function_->getBasicBlocks()) {
        auto* node = nodes_[bb->getName()];

        // 根据最后一条指令确定后继
        if (!bb->getInstructions().empty()) {
            auto* lastInst = bb->getInstructions().back();
            LLIRInstructionType type = lastInst->getType();

            if (type == LLIRInstructionType::Br) {
                // 无条件分支：唯一的后继
                if (!bb->getSuccessors().empty()) {
                    auto* succBB = bb->getSuccessors()[0];
                    if (succBB) {
                        auto* succNode = nodes_[succBB->getName()];
                        node->addSuccessor(succNode);
                        succNode->addPredecessor(node);
                    }
                }
            } else if (type == LLIRInstructionType::Ret) {
                // 返回指令：没有后继
            } else {
                // 其他指令：顺序执行到下一个基本块
                for (auto* succBB : bb->getSuccessors()) {
                    if (succBB) {
                        auto* succNode = nodes_[succBB->getName()];
                        node->addSuccessor(succNode);
                        succNode->addPredecessor(node);
                    }
                }
            }
        }
    }
}

void CFG::identifyEntryAndExit() {
    // 入口节点是函数的第一个基本块
    entryNode_ = nodes_[function_->getEntryBlock()->getName()];

    // 出口节点是没有后继的节点（包含return指令）
    exitNodes_.clear();
    for (const auto& [name, node] : nodes_) {
        if (node->getSuccessors().empty()) {
            exitNodes_.push_back(node);
        }
    }
}

void CFG::computeDominators() {
    if (!entryNode_) return;

    // 初始化：所有节点的支配集包含所有节点
    for (auto& [name, node] : nodes_) {
        if (node == entryNode_) {
            dominators_[node].insert(node);
        } else {
            for (const auto& [n, _] : nodes_) {
                dominators_[node].insert(_);
            }
        }
    }

    // 迭代计算支配关系直到不动点
    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& [name, node] : nodes_) {
            if (node == entryNode_) continue;

            // 计算前驱的支配集交集
            std::unordered_set<CFGNode*> intersection;
            if (!node->getPredecessors().empty()) {
                auto* firstPred = node->getPredecessors()[0];
                intersection = dominators_[firstPred];

                for (size_t i = 1; i < node->getPredecessors().size(); ++i) {
                    auto* pred = node->getPredecessors()[i];
                    std::unordered_set<CFGNode*> temp;
                    for (auto* n : intersection) {
                        if (dominators_[pred].count(n)) {
                            temp.insert(n);
                        }
                    }
                    intersection = std::move(temp);
                }
            }

            // 添加自身
            intersection.insert(node);

            // 检查是否变化
            if (intersection != dominators_[node]) {
                dominators_[node] = std::move(intersection);
                changed = true;
            }
        }
    }
}

void CFG::computePostDominators() {
    if (exitNodes_.empty()) return;

    // 反转CFG
    std::unordered_map<CFGNode*, std::vector<CFGNode*>> reverseEdges;
    for (auto& [name, node] : nodes_) {
        for (auto* succ : node->getSuccessors()) {
            reverseEdges[succ].push_back(node);
        }
    }

    // 初始化
    for (auto& [name, node] : nodes_) {
        bool isExit = std::find(exitNodes_.begin(), exitNodes_.end(), node) != exitNodes_.end();
        if (isExit) {
            postDominators_[node].insert(node);
        } else {
            for (const auto& [n, _] : nodes_) {
                postDominators_[node].insert(_);
            }
        }
    }

    // 迭代计算
    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& [name, node] : nodes_) {
            bool isExit = std::find(exitNodes_.begin(), exitNodes_.end(), node) != exitNodes_.end();
            if (isExit) continue;

            // 计算后继的后支配集交集
            std::unordered_set<CFGNode*> intersection;
            auto& succs = node->getSuccessors();
            if (!succs.empty()) {
                auto* firstSucc = succs[0];
                intersection = postDominators_[firstSucc];

                for (size_t i = 1; i < succs.size(); ++i) {
                    auto* succ = succs[i];
                    std::unordered_set<CFGNode*> temp;
                    for (auto* n : intersection) {
                        if (postDominators_[succ].count(n)) {
                            temp.insert(n);
                        }
                    }
                    intersection = std::move(temp);
                }
            }

            intersection.insert(node);

            if (intersection != postDominators_[node]) {
                postDominators_[node] = std::move(intersection);
                changed = true;
            }
        }
    }
}

bool CFG::dominates(CFGNode* a, CFGNode* b) const {
    if (a == b) return true;
    auto it = dominators_.find(b);
    return it != dominators_.end() && it->second.count(a);
}

bool CFG::postDominates(CFGNode* a, CFGNode* b) const {
    if (a == b) return true;
    auto it = postDominators_.find(b);
    return it != postDominators_.end() && it->second.count(a);
}

std::vector<CFGNode*> CFG::getDominanceFrontier(CFGNode* node) const {
    auto it = dominanceFrontier_.find(node);
    if (it != dominanceFrontier_.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::pair<CFGNode*, CFGNode*>> CFG::findBackEdges() const {
    std::vector<std::pair<CFGNode*, CFGNode*>> backEdges;

    // 使用DFS检测回边：从当前节点到其祖先节点的边
    std::unordered_map<CFGNode*, int> discoveryTime;
    std::unordered_map<CFGNode*, int> lowTime;
    std::unordered_set<CFGNode*> visited;
    int time = 0;

    std::function<void(CFGNode*)> dfs = [&](CFGNode* node) {
        discoveryTime[node] = lowTime[node] = time++;
        visited.insert(node);

        for (auto* succ : node->getSuccessors()) {
            if (!visited.count(succ)) {
                dfs(succ);
                lowTime[node] = std::min(lowTime[node], lowTime[succ]);

                // 检查是否是回边
                if (lowTime[succ] >= discoveryTime[node]) {
                    // succ到node的边是回边
                    backEdges.push_back({succ, node});
                }
            } else {
                lowTime[node] = std::min(lowTime[node], discoveryTime[succ]);
            }
        }
    };

    if (entryNode_) {
        dfs(entryNode_);
    }

    return backEdges;
}

bool CFG::hasPath(CFGNode* from, CFGNode* to) const {
    if (from == to) return true;

    std::unordered_set<CFGNode*> visited;
    std::queue<CFGNode*> queue;
    queue.push(from);
    visited.insert(from);

    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop();

        for (auto* succ : current->getSuccessors()) {
            if (succ == to) return true;
            if (!visited.count(succ)) {
                visited.insert(succ);
                queue.push(succ);
            }
        }
    }

    return false;
}

int CFG::computeDepth(CFGNode* node) const {
    if (node == entryNode_) return 0;

    std::unordered_map<CFGNode*, int> depth;
    std::queue<CFGNode*> queue;
    queue.push(entryNode_);
    depth[entryNode_] = 0;

    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop();

        for (auto* succ : current->getSuccessors()) {
            if (!depth.count(succ)) {
                depth[succ] = depth[current] + 1;
                queue.push(succ);
            }
        }
    }

    auto it = depth.find(node);
    return it != depth.end() ? it->second : -1;
}

std::vector<std::vector<CFGNode*>> CFG::findLoops() const {
    std::vector<std::vector<CFGNode*>> loops;
    auto backEdges = findBackEdges();

    for (auto [source, target] : backEdges) {
        // 找到从target到source的所有节点，形成一个自然循环
        std::vector<CFGNode*> loopNodes;
        std::unordered_set<CFGNode*> inLoop;

        std::function<void(CFGNode*)> collect = [&](CFGNode* node) {
            if (inLoop.count(node)) return;
            inLoop.insert(node);
            loopNodes.push_back(node);

            for (auto* pred : node->getPredecessors()) {
                if (pred != target && hasPath(pred, source)) {
                    collect(pred);
                }
            }
        };

        collect(target);
        loops.push_back(loopNodes);
    }

    return loops;
}

std::string CFG::toString() const {
    std::ostringstream oss;

    oss << "CFG for function: " << function_->getName() << "\n";
    oss << "Entry: " << (entryNode_ ? entryNode_->getId() : "null") << "\n";
    oss << "Exits: ";
    for (auto* exit : exitNodes_) {
        oss << exit->getId() << " ";
    }
    oss << "\n";

    oss << "Nodes:\n";
    for (const auto& [name, node] : nodes_) {
        oss << "  " << name << ":\n";
        oss << "    Successors: ";
        for (auto* succ : node->getSuccessors()) {
            oss << succ->getId() << " ";
        }
        oss << "\n    Predecessors: ";
        for (auto* pred : node->getPredecessors()) {
            oss << pred->getId() << " ";
        }
        oss << "\n";
    }

    return oss.str();
}

std::string CFG::toDOT() const {
    std::ostringstream oss;

    oss << "digraph CFG_" << function_->getName() << " {\n";
    oss << "  node [shape=rectangle];\n";

    // 添加节点
    for (const auto& [name, node] : nodes_) {
        oss << "  \"" << name << "\";\n";
    }

    // 添加边
    for (const auto& [name, node] : nodes_) {
        for (auto* succ : node->getSuccessors()) {
            oss << "  \"" << name << "\" -> \"" << succ->getId() << "\";\n";
        }
    }

    // 标记入口节点
    if (entryNode_) {
        oss << "  \"" << entryNode_->getId() << "\" [style=filled, fillcolor=lightgreen];\n";
    }

    // 标记出口节点
    for (auto* exit : exitNodes_) {
        oss << "  \"" << exit->getId() << "\" [style=filled, fillcolor=lightcoral];\n";
    }

    oss << "}\n";

    return oss.str();
}

// ============================================================================
// CFGTraversal 实现
// ============================================================================

std::vector<CFGNode*> CFGTraversal::preOrderTraversal() {
    std::vector<CFGNode*> result;
    std::unordered_set<CFGNode*> visited;

    if (cfg_->getEntryNode()) {
        preOrderDFS(cfg_->getEntryNode(), visited, result);
    }

    return result;
}

void CFGTraversal::preOrderDFS(
    CFGNode* node,
    std::unordered_set<CFGNode*>& visited,
    std::vector<CFGNode*>& result
) {
    if (!node || visited.count(node)) return;

    visited.insert(node);
    result.push_back(node);

    for (auto* succ : node->getSuccessors()) {
        preOrderDFS(succ, visited, result);
    }
}

std::vector<CFGNode*> CFGTraversal::postOrderTraversal() {
    std::vector<CFGNode*> result;
    std::unordered_set<CFGNode*> visited;

    if (cfg_->getEntryNode()) {
        postOrderDFS(cfg_->getEntryNode(), visited, result);
    }

    return result;
}

void CFGTraversal::postOrderDFS(
    CFGNode* node,
    std::unordered_set<CFGNode*>& visited,
    std::vector<CFGNode*>& result
) {
    if (!node || visited.count(node)) return;

    visited.insert(node);

    for (auto* succ : node->getSuccessors()) {
        postOrderDFS(succ, visited, result);
    }

    result.push_back(node);
}

std::vector<CFGNode*> CFGTraversal::reversePostOrderTraversal() {
    auto postOrder = postOrderTraversal();
    std::reverse(postOrder.begin(), postOrder.end());
    return postOrder;
}

std::vector<CFGNode*> CFGTraversal::bfsTraversal() {
    std::vector<CFGNode*> result;
    std::unordered_set<CFGNode*> visited;
    std::queue<CFGNode*> queue;

    if (cfg_->getEntryNode()) {
        queue.push(cfg_->getEntryNode());
        visited.insert(cfg_->getEntryNode());
    }

    while (!queue.empty()) {
        auto* node = queue.front();
        queue.pop();
        result.push_back(node);

        for (auto* succ : node->getSuccessors()) {
            if (!visited.count(succ)) {
                visited.insert(succ);
                queue.push(succ);
            }
        }
    }

    return result;
}

std::vector<CFGNode*> CFGTraversal::dfsTraversal() {
    return preOrderTraversal();
}

// ============================================================================
// PathCollector 实现
// ============================================================================

std::vector<ExecutionPath> PathCollector::collectAllPaths() {
    return collectAllPathsWithLimit(-1);  // 无限制
}

std::vector<ExecutionPath> PathCollector::collectAllPathsWithLimit(int maxDepth) {
    std::vector<ExecutionPath> paths;
    std::vector<CFGNode*> currentPath;
    std::unordered_set<CFGNode*> visited;

    if (cfg_->getEntryNode()) {
        dfsCollectPaths(cfg_->getEntryNode(), currentPath, visited, paths, 0, maxDepth);
    }

    return paths;
}

std::vector<ExecutionPath> PathCollector::collectPathsTo(CFGNode* target) {
    std::vector<ExecutionPath> paths;
    std::vector<CFGNode*> currentPath;
    std::unordered_set<CFGNode*> visited;

    std::function<void(CFGNode*)> dfs = [&](CFGNode* node) {
        if (!node || visited.count(node)) return;

        visited.insert(node);
        currentPath.push_back(node);

        if (node == target) {
            ExecutionPath path;
            for (auto* n : currentPath) {
                path.addNode(n);
            }
            paths.push_back(path);
        } else {
            for (auto* succ : node->getSuccessors()) {
                dfs(succ);
            }
        }

        currentPath.pop_back();
        visited.erase(node);
    };

    if (cfg_->getEntryNode()) {
        dfs(cfg_->getEntryNode());
    }

    return paths;
}

void PathCollector::dfsCollectPaths(
    CFGNode* current,
    std::vector<CFGNode*>& currentPath,
    std::unordered_set<CFGNode*>& visited,
    std::vector<ExecutionPath>& paths,
    int depth,
    int maxDepth
) {
    if (!current) return;

    // 检查深度限制
    if (maxDepth >= 0 && depth > maxDepth) {
        return;
    }

    currentPath.push_back(current);

    // 如果是出口节点，记录路径
    bool isExit = std::find(
        cfg_->getExitNodes().begin(),
        cfg_->getExitNodes().end(),
        current
    ) != cfg_->getExitNodes().end();

    if (isExit) {
        ExecutionPath path;
        for (auto* node : currentPath) {
            path.addNode(node);
        }
        paths.push_back(path);
    } else {
        // 继续探索后继
        for (auto* succ : current->getSuccessors()) {
            dfsCollectPaths(succ, currentPath, visited, paths, depth + 1, maxDepth);
        }
    }

    currentPath.pop_back();
}

} // namespace core
} // namespace cverifier
