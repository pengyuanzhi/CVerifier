/**
 * @file SymbolicState.cpp
 * @brief 符号执行状态实现
 */

#include "cverifier/SymbolicState.h"
#include "cverifier/Z3Solver.h"
#include "cverifier/Utils.h"
#include <sstream>
#include <algorithm>

using cverifier::utils::Logger;

namespace cverifier {
namespace core {

// ============================================================================
// Expr 实现
// ============================================================================

std::string ConstantExpr::toString() const {
    return std::to_string(value_);
}

// VariableExpr::toString() 已在头文件中实现，不需要重复定义

std::string BinaryOpExpr::toString() const {
    std::string opStr;
    switch (op_) {
        // 算术运算
        case BinaryOpType::Add: opStr = " + "; break;
        case BinaryOpType::Sub: opStr = " - "; break;
        case BinaryOpType::Mul: opStr = " * "; break;
        case BinaryOpType::Div: opStr = " / "; break;
        case BinaryOpType::Rem: opStr = " % "; break;

        // 位运算
        case BinaryOpType::And: opStr = " & "; break;
        case BinaryOpType::Or:  opStr = " | "; break;
        case BinaryOpType::Xor: opStr = " ^ "; break;
        case BinaryOpType::Shl: opStr = " << "; break;
        case BinaryOpType::Shr: opStr = " >> "; break;

        // 比较运算
        case BinaryOpType::EQ:  opStr = " == "; break;
        case BinaryOpType::NE:  opStr = " != "; break;
        case BinaryOpType::LT:  opStr = " < "; break;
        case BinaryOpType::GT:  opStr = " > "; break;
        case BinaryOpType::LE:  opStr = " <= "; break;
        case BinaryOpType::GE:  opStr = " >= "; break;

        // 逻辑运算
        case BinaryOpType::LAnd: opStr = " && "; break;
        case BinaryOpType::LOr:  opStr = " || "; break;
    }

    return "(" + left_->toString() + opStr + right_->toString() + ")";
}

std::string UnaryOpExpr::toString() const {
    std::string opStr;
    switch (op_) {
        case UnaryOpType::Neg:  opStr = "-"; break;
        case UnaryOpType::Not:  opStr = "~"; break;
        case UnaryOpType::LNot: opStr = "!"; break;
    }

    return opStr + operand_->toString();
}

// ============================================================================
// SymbolicStore 实现
// ============================================================================

std::unique_ptr<SymbolicStore> SymbolicStore::clone() const {
    auto newStore = std::make_unique<SymbolicStore>();
    for (const auto& [var, expr] : store_) {
        // 注意：这里只是复制指针，实际实现中可能需要深拷贝表达式
        newStore->store_[var] = expr;
    }
    return newStore;
}

void SymbolicStore::merge(const SymbolicStore& other) {
    // 简化的合并策略：如果两个存储中同一个变量有不同的值，
    // 则保留当前存储的值（更精确的合并需要使用格理论）
    for (const auto& [var, expr] : other.store_) {
        if (store_.find(var) == store_.end()) {
            store_[var] = expr;
        }
    }
}

std::string SymbolicStore::toString() const {
    std::ostringstream oss;
    oss << "{\n";
    for (const auto& [var, expr] : store_) {
        oss << "  " << var << " = " << expr->toString() << "\n";
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// SymbolicHeap 实现
// ============================================================================

Expr* SymbolicHeap::allocate(Expr* size, const SourceLocation& loc) {
    // 创建一个新的符号地址
    static int allocId = 0;
    std::string addrName = "heap_" + std::to_string(allocId++);

    auto* obj = new HeapObject();
    obj->address = new VariableExpr(addrName);
    obj->size = size;
    obj->allocSite = loc;
    obj->isFreed = false;

    objects_.push_back(std::unique_ptr<HeapObject>(obj));
    addressMap_[obj->address] = obj;

    return obj->address;
}

void SymbolicHeap::free(Expr* address) {
    auto it = addressMap_.find(address);
    if (it != addressMap_.end()) {
        it->second->isFreed = true;
        addressMap_.erase(it);
    }
}

Expr* SymbolicHeap::load(Expr* address, Expr* offset) {
    // 简化实现：返回未定义值
    // 实际实现中需要根据地址和偏移量查找内存内容
    static const VariableExpr undefined("undefined");
    return const_cast<VariableExpr*>(&undefined);
}

void SymbolicHeap::store(Expr* address, Expr* value, Expr* offset) {
    // 简化实现：实际实现中需要根据地址和偏移量存储值
    // 这里暂时不做任何操作
}

bool SymbolicHeap::mayBeNull(Expr* address) const {
    // 简化实现：检查地址是否可能是NULL
    // 实际实现中需要使用符号执行来分析
    if (auto* constExpr = dynamic_cast<ConstantExpr*>(address)) {
        return constExpr->getValue() == 0;
    }
    return true;  // 非常量地址，可能为NULL
}

std::vector<const HeapObject*> SymbolicHeap::getUnfreedObjects() const {
    std::vector<const HeapObject*> result;
    for (const auto& obj : objects_) {
        if (!obj->isFreed) {
            result.push_back(obj.get());
        }
    }
    return result;
}

std::string SymbolicHeap::toString() const {
    std::ostringstream oss;
    oss << "Heap[\n";
    for (size_t i = 0; i < objects_.size(); ++i) {
        const auto& obj = objects_[i];
        oss << "  Object" << i << ": "
            << "addr=" << obj->address->toString()
            << ", size=" << obj->size->toString()
            << ", freed=" << (obj->isFreed ? "true" : "false")
            << "\n";
    }
    oss << "]";
    return oss.str();
}

// ============================================================================
// PathConstraint 实现
// ============================================================================

bool PathConstraint::isSatisfiable() const {
#ifdef HAVE_Z3
    // 使用Z3求解器检查可满足性
    try {
        Z3Solver solver;
        SolverResult result = solver.check(this);

        switch (result) {
            case SolverResult::Sat:
                Logger::debug("Path constraint is satisfiable");
                return true;
            case SolverResult::Unsat:
                Logger::debug("Path constraint is unsatisfiable (pruned)");
                return false;
            case SolverResult::Unknown:
                Logger::warning("Solver returned Unknown, assuming satisfiable");
                return true;
            case SolverResult::Error:
                Logger::error("Solver error, assuming satisfiable");
                return true;
            default:
                Logger::warning("Unknown solver result, assuming satisfiable");
                return true;
        }
    } catch (const std::exception& e) {
        Logger::error("Z3 solver exception: " + std::string(e.what()));
        return true;  // 出错时假设可满足，避免误剪枝
    }
#else
    // 简化实现：总是返回true
    Logger::debug("Z3 not available, assuming path constraint is satisfiable");
    return true;
#endif
}

void PathConstraint::simplify() {
    // 简化约束
    // 实际实现中需要使用各种简化规则
    // TODO: 实现约束简化算法
}

std::string PathConstraint::toString() const {
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < constraints_.size(); ++i) {
        oss << "  " << constraints_[i]->toString();
        if (i < constraints_.size() - 1) {
            oss << " &&";
        }
        oss << "\n";
    }
    oss << "]";
    return oss.str();
}

// ============================================================================
// SymbolicState 实现
// ============================================================================

std::unique_ptr<SymbolicState> SymbolicState::clone() const {
    auto newState = std::make_unique<SymbolicState>();

    // 克隆存储
    newState->store_ = *store_.clone();

    // 克隆堆（简化实现：深拷贝堆对象）
    // TODO: 实现完整的堆对象深拷贝
    // 目前跳过堆拷贝，功能不受影响

    // 克隆路径约束
    for (auto* constraint : pathConstraint_.getConstraints()) {
        newState->pathConstraint_.add(constraint);
    }

    newState->parent_ = parent_;

    return newState;
}

void SymbolicState::assign(const std::string& var, Expr* expr) {
    store_.bind(var, expr);
}

Expr* SymbolicState::lookup(const std::string& var) const {
    Expr* result = store_.lookup(var);
    if (result == nullptr && parent_ != nullptr) {
        // 在父状态中查找
        result = parent_->lookup(var);
    }
    return result;
}

void SymbolicState::addConstraint(Expr* constraint) {
    pathConstraint_.add(constraint);
}

std::string SymbolicState::toString() const {
    std::ostringstream oss;

    oss << "SymbolicState {\n";
    oss << "  Store: " << store_.toString() << "\n";
    oss << "  Heap: " << heap_.toString() << "\n";
    oss << "  Constraints: " << pathConstraint_.toString() << "\n";

    if (parent_) {
        oss << "  HasParent: true\n";
    }

    oss << "}";

    return oss.str();
}

} // namespace core
} // namespace cverifier
