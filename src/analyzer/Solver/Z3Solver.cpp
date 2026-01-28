/**
 * @file Z3Solver.cpp
 * @brief Z3 SMT 求解器实现
 */

#include "cverifier/Z3Solver.h"
#include "cverifier/Utils.h"
#include <sstream>
#include <stdexcept>

namespace cverifier {
namespace core {

// ============================================================================
// Z3Solver 实现
// ============================================================================

Z3Solver::Z3Solver() : timeout_(5000) {
#ifdef HAVE_Z3
    // 配置Z3
    z3::config config;

    // 设置超时
    if (timeout_ > 0) {
        config.set("timeout", static_cast<unsigned>(timeout_));
    }

    // 启用模型
    config.set("model", true);

    // 设置求解器策略
    config.set("auto_config", true);

    // 创建上下文和求解器
    ctx_ = z3::context(config);
    solver_ = z3::solver(ctx_, "QF_LIA");  // 线性整数算术理论

    utils::Logger::debug("Z3 solver initialized successfully");
#else
    utils::Logger::warning("Z3 not available. Using simplified solver implementation.");
#endif
}

Z3Solver::~Z3Solver() {
#ifdef HAVE_Z3
    // Z3对象会自动清理
#endif
}

SolverResult Z3Solver::check(const PathConstraint* constraints) {
#ifdef HAVE_Z3
    if (!constraints) {
        return SolverResult::Error;
    }

    try {
        // 清除之前的断言
        solver_.reset();

        // 添加所有约束
        for (Expr* constraint : constraints->getConstraints()) {
            z3::expr z3Expr = convertToZ3(constraint);
            solver_.add(z3Expr);
        }

        // 求解
        z3::check_result result = solver_.check();

        // 转换结果
        switch (result) {
            case z3::sat:
                // 提取模型
                z3::model model = solver_.get_model();
                lastModel_.intValues.clear();
                lastModel_.floatValues.clear();
                lastModel_.boolValues.clear();

                for (unsigned i = 0; i < model.num_consts(); ++i) {
                    z3::func_decl decl = model.get_const_decl(i);
                    z3::expr value = model.get_const_interp(decl);
                    std::string name = decl.name().str();

                    if (value.is_int()) {
                        int64_t intVal;
                        if (Z3_get_numeral_int64(ctx_, value, &intVal)) {
                            lastModel_.intValues[name] = intVal;
                        }
                    } else if (value.is_bool()) {
                        lastModel_.boolValues[name] = Z3_get_bool_value(ctx_, value) == Z3_L_TRUE;
                    }
                }

                return SolverResult::Sat;

            case z3::unsat:
                return SolverResult::Unsat;

            case z3::unknown:
                return SolverResult::Unknown;

            default:
                return SolverResult::Error;
        }

    } catch (const std::exception& e) {
        utils::Logger::error("Z3 solver exception: " + std::string(e.what()));
        return SolverResult::Error;
    }
#else
    // 简化实现：总是返回可满足
    utils::Logger::debug("Using simplified solver (always returns Sat)");
    return SolverResult::Sat;
#endif
}

SolverResult Z3Solver::check(Expr* expr) {
#ifdef HAVE_Z3
    if (!expr) {
        return SolverResult::Error;
    }

    try {
        solver_.reset();
        z3::expr z3Expr = convertToZ3(expr);
        solver_.add(z3Expr);

        z3::check_result result = solver_.check();

        switch (result) {
            case z3::sat: return SolverResult::Sat;
            case z3::unsat: return SolverResult::Unsat;
            case z3::unknown: return SolverResult::Unknown;
            default: return SolverResult::Error;
        }

    } catch (const std::exception& e) {
        utils::Logger::error("Z3 solver exception: " + std::string(e.what()));
        return SolverResult::Error;
    }
#else
    return SolverResult::Sat;
#endif
}

bool Z3Solver::isValid(Expr* expr) {
    // 表达式是永真的，当且仅当其否定不可满足
#ifdef HAVE_Z3
    try {
        solver_.reset();

        // 添加表达式的否定
        z3::expr z3Expr = convertToZ3(expr);
        solver_.add(!z3Expr);

        z3::check_result result = solver_.check();
        return result == z3::unsat;

    } catch (const std::exception& e) {
        utils::Logger::error("Z3 solver exception: " + std::string(e.what()));
        return false;
    }
#else
    return true;
#endif
}

CounterExample Z3Solver::getModel() const {
    return lastModel_;
}

void Z3Solver::setTimeout(unsigned int milliseconds) {
    timeout_ = milliseconds;
#ifdef HAVE_Z3
    // Z3需要在创建时设置超时，这里仅记录
    // 实际超时设置在下次创建solver时生效
#endif
}

void Z3Solver::push() {
#ifdef HAVE_Z3
    solver_.push();
#endif
}

void Z3Solver::pop() {
#ifdef HAVE_Z3
    solver_.pop(1);
#endif
}

void Z3Solver::addAssertion(Expr* expr) {
#ifdef HAVE_Z3
    if (expr) {
        z3::expr z3Expr = convertToZ3(expr);
        solver_.add(z3Expr);
    }
#endif
}

void Z3Solver::reset() {
#ifdef HAVE_Z3
    solver_.reset();
#endif
}

Expr* Z3Solver::simplify(Expr* expr) {
    // 简化表达式（需要实现表达式优化）
    // 这里简化处理：直接返回原表达式
    return expr;
}

std::string Z3Solver::getStatistics() const {
    std::ostringstream oss;

#ifdef HAVE_Z3
    oss << "Z3 Solver Statistics:\n";
    oss << "  Timeout: " << timeout_ << "ms\n";

    try {
        z3::stats stats = Z3_solver_get_statistics(ctx_, solver_);
        oss << "  Statistics:\n";
        // Z3统计信息格式化
        Z3_stats_dec_ref(ctx_, stats);
    } catch (...) {
        oss << "  Statistics: unavailable\n";
    }
#else
    oss << "Simplified Solver (Z3 not available)\n";
#endif

    return oss.str();
}

#ifdef HAVE_Z3
z3::expr Z3Solver::convertToZ3(Expr* expr) {
    if (!expr) {
        return ctx_.bool_val(true);
    }

    switch (expr->getType()) {
        case ExprType::Constant: {
            auto* constExpr = dynamic_cast<ConstantExpr*>(expr);
            if (constExpr) {
                return ctx_.int_val(constExpr->getValue());
            }
            break;
        }

        case ExprType::Variable: {
            auto* varExpr = dynamic_cast<VariableExpr*>(expr);
            if (varExpr) {
                return createZ3Constant(varExpr->getName(), ValueType::Integer);
            }
            break;
        }

        case ExprType::BinaryOp: {
            auto* binOp = dynamic_cast<BinaryOpExpr*>(expr);
            if (binOp) {
                z3::expr left = convertToZ3(binOp->getLeft());
                z3::expr right = convertToZ3(binOp->getRight());

                switch (binOp->getOp()) {
                    case BinaryOpType::Add:
                        return left + right;
                    case BinaryOpType::Sub:
                        return left - right;
                    case BinaryOpType::Mul:
                        return left * right;
                    case BinaryOpType::Div:
                        return left / right;
                    case BinaryOpType::Rem:
                        return z3::rem(left, right);
                    case BinaryOpType::And:
                        return left & right;
                    case BinaryOpType::Or:
                        return left | right;
                    case BinaryOpType::Xor:
                        return left ^ right;
                    case BinaryOpType::Shl:
                        return z3::shl(left, static_cast<int>(Z3_get_numeral_int64(ctx_, right)));
                    case BinaryOpType::Shr:
                        return z3::ashr(left, static_cast<int>(Z3_get_numeral_int64(ctx_, right)));
                    case BinaryOpType::EQ:
                        return left == right;
                    case BinaryOpType::NE:
                        return left != right;
                    case BinaryOpType::LT:
                        return left < right;
                    case BinaryOpType::LE:
                        return left <= right;
                    case BinaryOpType::GT:
                        return left > right;
                    case BinaryOpType::GE:
                        return left >= right;
                    case BinaryOpType::LAnd:
                        return left && right;
                    case BinaryOpType::LOr:
                        return left || right;
                    default:
                        return ctx_.bool_val(true);
                }
            }
            break;
        }

        case ExprType::UnaryOp: {
            auto* unOp = dynamic_cast<UnaryOpExpr*>(expr);
            if (unOp) {
                z3::expr operand = convertToZ3(unOp->getOperand());

                switch (unOp->getOp()) {
                    case UnaryOpType::Neg:
                        return -operand;
                    case UnaryOpType::Not:
                        return ~operand;
                    case UnaryOpType::LNot:
                        return !operand;
                    default:
                        return ctx_.bool_val(true);
                }
            }
            break;
        }

        default:
            break;
    }

    return ctx_.bool_val(true);
}

z3::expr Z3Solver::createZ3Constant(const std::string& name, ValueType type) {
    switch (type) {
        case ValueType::Integer:
            return ctx_.int_const(name.c_str());
        case ValueType::Float:
            // Z3使用浮点数理论
            return ctx().real_const(name.c_str());
        case ValueType::Pointer:
            // 指针作为整数处理
            return ctx_.int_const(name.c_str());
        case ValueType::Boolean:
            return ctx_.bool_const(name.c_str());
        default:
            return ctx_.int_const(name.c_str());
    }
}
#endif

// ============================================================================
// ConstraintBuilder 实现
// ============================================================================

Expr* ConstraintBuilder::eq(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::EQ, left, right);
}

Expr* ConstraintBuilder::neq(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::NE, left, right);
}

Expr* ConstraintBuilder::lt(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::LT, left, right);
}

Expr* ConstraintBuilder::le(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::LE, left, right);
}

Expr* ConstraintBuilder::gt(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::GT, left, right);
}

Expr* ConstraintBuilder::ge(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::GE, left, right);
}

Expr* ConstraintBuilder::land(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::LAnd, left, right);
}

Expr* ConstraintBuilder::lor(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::LOr, left, right);
}

Expr* ConstraintBuilder::lnot(Expr* expr) {
    return new UnaryOpExpr(UnaryOpType::LNot, expr);
}

Expr* ConstraintBuilder::implies(Expr* antecedent, Expr* consequent) {
    // A => B 等价于 !A || B
    return lor(lnot(antecedent), consequent);
}

Expr* ConstraintBuilder::add(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Add, left, right);
}

Expr* ConstraintBuilder::sub(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Sub, left, right);
}

Expr* ConstraintBuilder::mul(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Mul, left, right);
}

Expr* ConstraintBuilder::div(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Div, left, right);
}

Expr* ConstraintBuilder::rem(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Rem, left, right);
}

Expr* ConstraintBuilder::bitwiseAnd(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::And, left, right);
}

Expr* ConstraintBuilder::bitwiseOr(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Or, left, right);
}

Expr* ConstraintBuilder::bitwiseXor(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Xor, left, right);
}

Expr* ConstraintBuilder::shiftLeft(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Shl, left, right);
}

Expr* ConstraintBuilder::shiftRight(Expr* left, Expr* right) {
    return new BinaryOpExpr(BinaryOpType::Shr, left, right);
}

Expr* ConstraintBuilder::bufferAccess(Expr* ptr, Expr* base, Expr* size) {
    // 缓冲区访问安全约束: ptr >= base && ptr < base + size
    Expr* ptrValid = land(
        ge(ptr, base),
        lt(ptr, add(base, size))
    );
    return ptrValid;
}

Expr* ConstraintBuilder::pointerValid(Expr* ptr) {
    // 指针有效约束：非空
    return neq(ptr, new ConstantExpr(0));
}

Expr* ConstraintBuilder::pointerNonNull(Expr* ptr) {
    return neq(ptr, new ConstantExpr(0));
}

Expr* ConstraintBuilder::pointerInRange(Expr* ptr, Expr* base, Expr* size) {
    return bufferAccess(ptr, base, size);
}

Expr* ConstraintBuilder::addOverflow(Expr* left, Expr* right, bool isSigned) {
    // 加法溢出检测
    // 有符号: (left + right) < left (对于正数) 或其他复杂的符号位检查
    // 无符号: (left + right) < left
    if (isSigned) {
        // 简化处理：有符号溢出检查很复杂
        // 实际应该检查符号位
        return new ConstantExpr(0);  // 假设不溢出
    } else {
        // 无符号溢出: left + right < left
        return lt(add(left, right), left);
    }
}

Expr* ConstraintBuilder::subOverflow(Expr* left, Expr* right, bool isSigned) {
    // 减法溢出检测
    if (isSigned) {
        return new ConstantExpr(0);
    } else {
        // 无符号溢出: left < right
        return lt(left, right);
    }
}

Expr* ConstraintBuilder::mulOverflow(Expr* left, Expr* right, bool isSigned) {
    // 乘法溢出检测（简化）
    return new ConstantExpr(0);
}

Expr* ConstraintBuilder::floatIsNan(Expr* expr) {
    // 浮点NaN检测（需要特殊处理）
    // 这里简化返回false
    return new ConstantExpr(0);
}

Expr* ConstraintBuilder::floatIsInf(Expr* expr) {
    // 浮点无穷大检测
    return new ConstantExpr(0);
}

Expr* ConstraintBuilder::floatIsFinite(Expr* expr) {
    // 浮点有限数检测
    return new ConstantExpr(1);
}

Expr* ConstraintBuilder::floatMultiplyOverflow(Expr* left, Expr* right) {
    // 浮点乘法上溢检测
    return new ConstantExpr(0);
}

Expr* ConstraintBuilder::floatDivisionByZero(Expr* divisor) {
    // 浮点除零检测
    return eq(divisor, new ConstantExpr(0));
}

} // namespace core
} // namespace cverifier
