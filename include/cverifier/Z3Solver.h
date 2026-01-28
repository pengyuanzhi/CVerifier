#ifndef CVERIFIER_Z3_SOLVER_H
#define CVERIFIER_Z3_SOLVER_H

#include "cverifier/Core.h"
#include "cverifier/SymbolicState.h"
#include <string>
#include <vector>
#include <memory>

#ifdef HAVE_Z3
#include <z3++.h>
#endif

namespace cverifier {
namespace core {

// ============================================================================
// 求解结果
// ============================================================================

/**
 * @brief 求解结果类型
 */
enum class SolverResult {
    Sat,        ///< 可满足（存在反例）
    Unsat,      ///< 不可满足（路径不可行）
    Unknown,    ///< 未知（超时或求解器限制）
    Error       ///< 错误
};

/**
 * @brief 反例（模型）
 */
struct CounterExample {
    std::unordered_map<std::string, int64_t> intValues;
    std::unordered_map<std::string, double> floatValues;
    std::unordered_map<std::string, bool> boolValues;

    std::string toString() const {
        std::ostringstream oss;
        oss << "Counter Example:\n";

        for (const auto& [var, val] : intValues) {
            oss << "  " << var << " = " << val << "\n";
        }

        for (const auto& [var, val] : floatValues) {
            oss << "  " << var << " = " << val << "\n";
        }

        for (const auto& [var, val] : boolValues) {
            oss << "  " << var << " = " << (val ? "true" : "false") << "\n";
        }

        return oss.str();
    }
};

// ============================================================================
// Z3 求解器
// ============================================================================

/**
 * @brief Z3 SMT 求解器封装
 *
 * 提供符号表达式到Z3约束的转换和求解功能
 */
class Z3Solver {
public:
    Z3Solver();
    ~Z3Solver();

    // 禁止拷贝
    Z3Solver(const Z3Solver&) = delete;
    Z3Solver& operator=(const Z3Solver&) = delete;

    /**
     * @brief 检查路径约束的可满足性
     */
    SolverResult check(const PathConstraint* constraints);

    /**
     * @brief 检查单个表达式的可满足性
     */
    SolverResult check(Expr* expr);

    /**
     * @brief 检查表达式是否为永真式
     */
    bool isValid(Expr* expr);

    /**
     * @brief 获取反例（仅在Sat结果时有意义）
     */
    CounterExample getModel() const;

    /**
     * @brief 设置超时时间（毫秒）
     */
    void setTimeout(unsigned int milliseconds);

    /**
     * @brief 推送约束上下文（用于增量求解）
     */
    void push();

    /**
     * @brief 弹出约束上下文
     */
    void pop();

    /**
     * @brief 添加断言
     */
    void addAssertion(Expr* expr);

    /**
     * @brief 清除所有断言
     */
    void reset();

    /**
     * @brief 简化表达式
     */
    Expr* simplify(Expr* expr);

    /**
     * @brief 获取求解器统计信息
     */
    std::string getStatistics() const;

#ifdef HAVE_Z3
    /**
     * @brief 获取底层Z3上下文（用于高级操作）
     */
    z3::context& getContext() { return ctx_; }

    /**
     * @brief 获取底层Z3求解器
     */
    z3::solver& getSolver() { return solver_; }
#endif

private:
#ifdef HAVE_Z3
    /**
     * @brief 将符号表达式转换为Z3表达式
     */
    z3::expr convertToZ3(Expr* expr);

    /**
     * @brief 创建Z3常量
     */
    z3::expr createZ3Constant(const std::string& name, ValueType type);

    z3::context ctx_;
    z3::solver solver_;
    CounterExample lastModel_;
#else
    // 当Z3不可用时，使用简化实现
    CounterExample lastModel_;
#endif

    unsigned int timeout_;
};

// ============================================================================
// 约束构建器
// ============================================================================

/**
 * @brief 约束构建器
 *
 * 辅助构建复杂的符号约束表达式
 */
class ConstraintBuilder {
public:
    ConstraintBuilder() = default;

    /**
     * @brief 创建相等约束
     */
    static Expr* eq(Expr* left, Expr* right);

    /**
     * @brief 创建不等约束
     */
    static Expr* neq(Expr* left, Expr* right);

    /**
     * @brief 创建小于约束
     */
    static Expr* lt(Expr* left, Expr* right);

    /**
     * @brief 创建小于等于约束
     */
    static Expr* le(Expr* left, Expr* right);

    /**
     * @brief 创建大于约束
     */
    static Expr* gt(Expr* left, Expr* right);

    /**
     * @brief 创建大于等于约束
     */
    static Expr* ge(Expr* left, Expr* right);

    /**
     * @brief 创建逻辑与约束
     */
    static Expr* land(Expr* left, Expr* right);

    /**
     * @brief 创建逻辑或约束
     */
    static Expr* lor(Expr* left, Expr* right);

    /**
     * @brief 创建逻辑非约束
     */
    static Expr* lnot(Expr* expr);

    /**
     * @brief 创建蕴含约束
     */
    static Expr* implies(Expr* antecedent, Expr* consequent);

    /**
     * @brief 创建算术约束
     */
    static Expr* add(Expr* left, Expr* right);
    static Expr* sub(Expr* left, Expr* right);
    static Expr* mul(Expr* left, Expr* right);
    static Expr* div(Expr* left, Expr* right);
    static Expr* rem(Expr* left, Expr* right);

    /**
     * @brief 创建位运算约束
     */
    static Expr* bitwiseAnd(Expr* left, Expr* right);
    static Expr* bitwiseOr(Expr* left, Expr* right);
    static Expr* bitwiseXor(Expr* left, Expr* right);
    static Expr* shiftLeft(Expr* left, Expr* right);
    static Expr* shiftRight(Expr* left, Expr* right);

    /**
     * @brief 创建数组/缓冲区约束
     */
    static Expr* bufferAccess(Expr* ptr, Expr* base, Expr* size);
    static Expr* pointerValid(Expr* ptr);
    static Expr* pointerNonNull(Expr* ptr);
    static Expr* pointerInRange(Expr* ptr, Expr* base, Expr* size);

    /**
     * @brief 创建算术溢出约束
     */
    static Expr* addOverflow(Expr* left, Expr* right, bool isSigned);
    static Expr* subOverflow(Expr* left, Expr* right, bool isSigned);
    static Expr* mulOverflow(Expr* left, Expr* right, bool isSigned);

    /**
     * @brief 创建浮点约束
     */
    static Expr* floatIsNan(Expr* expr);
    static Expr* floatIsInf(Expr* expr);
    static Expr* floatIsFinite(Expr* expr);
    static Expr* floatMultiplyOverflow(Expr* left, Expr* right);
    static Expr* floatDivisionByZero(Expr* divisor);
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_Z3_SOLVER_H
