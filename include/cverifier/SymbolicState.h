#ifndef CVERIFIER_SYMBOLIC_STATE_H
#define CVERIFIER_SYMBOLIC_STATE_H

#include "cverifier/Core.h"
#include "cverifier/LLIRModule.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace cverifier {
namespace core {

// ============================================================================
// 符号表达式
// ============================================================================

/**
 * @brief 符号表达式类型
 */
enum class ExprType {
    Constant,      ///< 常量
    Variable,      ///< 变量
    BinaryOp,      ///< 二元操作
    UnaryOp,       ///< 一元操作
    Boolean,       ///< 布尔值
    NullPtr,       ///< 空指针
    Undefined      ///< 未定义
};

/**
 * @brief 二元操作类型
 */
enum class BinaryOpType {
    // 算术运算
    Add, Sub, Mul, Div, Rem,

    // 位运算
    And, Or, Xor, Shl, Shr,

    // 比较运算
    EQ, NE, LT, GT, LE, GE,

    // 逻辑运算
    LAnd, LOr
};

/**
 * @brief 一元操作类型
 */
enum class UnaryOpType {
    Neg,     ///< 取负
    Not,     ///< 按位取反
    LNot     ///< 逻辑取反
};

/**
 * @brief 符号表达式基类
 */
class Expr {
public:
    virtual ~Expr() = default;

    ExprType getType() const { return type_; }

    virtual std::string toString() const = 0;
    virtual bool isConstant() const { return false; }

protected:
    Expr(ExprType type) : type_(type) {}

    ExprType type_;
};

/**
 * @brief 常量表达式
 */
class ConstantExpr : public Expr {
public:
    explicit ConstantExpr(int64_t value)
        : Expr(ExprType::Constant), value_(value) {}

    int64_t getValue() const { return value_; }
    bool isConstant() const override { return true; }

    std::string toString() const override;

private:
    int64_t value_;
};

/**
 * @brief 变量表达式
 */
class VariableExpr : public Expr {
public:
    explicit VariableExpr(const std::string& name)
        : Expr(ExprType::Variable), name_(name) {}

    const std::string& getName() const { return name_; }
    std::string toString() const override { return name_; }

private:
    std::string name_;
};

/**
 * @brief 二元操作表达式
 */
class BinaryOpExpr : public Expr {
public:
    BinaryOpExpr(BinaryOpType op, Expr* left, Expr* right)
        : Expr(ExprType::BinaryOp), op_(op), left_(left), right_(right) {}

    BinaryOpType getOp() const { return op_; }
    Expr* getLeft() const { return left_; }
    Expr* getRight() const { return right_; }

    std::string toString() const override;

private:
    BinaryOpType op_;
    Expr* left_;
    Expr* right_;
};

/**
 * @brief 一元操作表达式
 */
class UnaryOpExpr : public Expr {
public:
    UnaryOpExpr(UnaryOpType op, Expr* operand)
        : Expr(ExprType::UnaryOp), op_(op), operand_(operand) {}

    UnaryOpType getOp() const { return op_; }
    Expr* getOperand() const { return operand_; }

    std::string toString() const override;

private:
    UnaryOpType op_;
    Expr* operand_;
};

// ============================================================================
// 符号存储
// ============================================================================

/**
 * @brief 符号存储（变量到表达式的映射）
 */
class SymbolicStore {
public:
    SymbolicStore() = default;

    /**
     * @brief 绑定变量到表达式
     */
    void bind(const std::string& var, Expr* expr) {
        store_[var] = expr;
    }

    /**
     * @brief 查找变量的值
     */
    Expr* lookup(const std::string& var) const {
        auto it = store_.find(var);
        return it != store_.end() ? it->second : nullptr;
    }

    /**
     * @brief 克隆当前存储
     */
    std::unique_ptr<SymbolicStore> clone() const;

    /**
     * @brief 合并两个存储
     */
    void merge(const SymbolicStore& other);

    std::string toString() const;

private:
    std::unordered_map<std::string, Expr*> store_;
};

// ============================================================================
// 符号堆
// ============================================================================

/**
 * @brief 堆对象
 */
struct HeapObject {
    Expr* address;           ///< 符号地址
    Expr* size;              ///< 对象大小
    SourceLocation allocSite; ///< 分配位置
    bool isFreed;            ///< 是否已释放

    HeapObject() : isFreed(false) {}
};

/**
 * @brief 符号堆
 */
class SymbolicHeap {
public:
    SymbolicHeap() = default;

    /**
     * @brief 分配内存
     */
    Expr* allocate(Expr* size, const SourceLocation& loc);

    /**
     * @brief 释放内存
     */
    void free(Expr* address);

    /**
     * @brief 读取内存
     */
    Expr* load(Expr* address, Expr* offset = nullptr);

    /**
     * @brief 写入内存
     */
    void store(Expr* address, Expr* value, Expr* offset = nullptr);

    /**
     * @brief 检查地址是否可能为空
     */
    bool mayBeNull(Expr* address) const;

    /**
     * @brief 获取所有未释放的对象
     */
    std::vector<const HeapObject*> getUnfreedObjects() const;

    std::string toString() const;

private:
    std::vector<std::unique_ptr<HeapObject>> objects_;
    std::unordered_map<Expr*, HeapObject*> addressMap_;
};

// ============================================================================
// 路径约束
// ============================================================================

/**
 * @brief 路径约束（符号执行过程中收集的条件）
 */
class PathConstraint {
public:
    PathConstraint() = default;

    /**
     * @brief 添加约束
     */
    void add(Expr* constraint) {
        constraints_.push_back(constraint);
    }

    /**
     * @brief 获取所有约束
     */
    const std::vector<Expr*>& getConstraints() const {
        return constraints_;
    }

    /**
     * @brief 检查约束是否可满足（需要SMT求解器）
     */
    bool isSatisfiable() const;

    /**
     * @brief 简化约束
     */
    void simplify();

    std::string toString() const;

private:
    std::vector<Expr*> constraints_;
};

// ============================================================================
// 符号状态
// ============================================================================

/**
 * @brief 符号执行状态
 */
class SymbolicState {
public:
    SymbolicState()
        : parent_(nullptr) {}

    SymbolicState(SymbolicState* parent)
        : parent_(parent) {}

    ~SymbolicState() = default;

    /**
     * @brief 获取符号存储
     */
    SymbolicStore* getStore() { return &store_; }
    const SymbolicStore* getStore() const { return &store_; }

    /**
     * @brief 获取符号堆
     */
    SymbolicHeap* getHeap() { return &heap_; }
    const SymbolicHeap* getHeap() const { return &heap_; }

    /**
     * @brief 获取路径约束
     */
    PathConstraint* getPathConstraint() { return &pathConstraint_; }
    const PathConstraint* getPathConstraint() const { return &pathConstraint_; }

    /**
     * @brief 克隆当前状态
     */
    std::unique_ptr<SymbolicState> clone() const;

    /**
     * @brief 赋值操作
     */
    void assign(const std::string& var, Expr* expr);

    /**
     * @brief 查找变量
     */
    Expr* lookup(const std::string& var) const;

    /**
     * @brief 添加路径约束
     */
    void addConstraint(Expr* constraint);

    /**
     * @brief 获取父状态
     */
    SymbolicState* getParent() const { return parent_; }

    /**
     * @brief 设置父状态
     */
    void setParent(SymbolicState* parent) { parent_ = parent; }

    std::string toString() const;

private:
    SymbolicStore store_;
    SymbolicHeap heap_;
    PathConstraint pathConstraint_;
    SymbolicState* parent_;
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_SYMBOLIC_STATE_H
