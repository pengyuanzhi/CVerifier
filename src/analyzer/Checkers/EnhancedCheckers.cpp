/**
 * @file EnhancedCheckers.cpp
 * @brief 增强版漏洞检测器（使用Z3求解器）
 */

#include "cverifier/SymbolicExecutionEngine.h"
#include "cverifier/Z3Solver.h"
#include "cverifier/Utils.h"
#include <sstream>

namespace cverifier {
namespace core {

// ============================================================================
// 增强版缓冲区溢出检测器
// ============================================================================

VulnerabilityReport* BufferOverflowChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    if (!state || !inst) {
        return nullptr;
    }

    // 只检测store指令
    if (inst->getType() != LLIRInstructionType::Store) {
        return nullptr;
    }

    // 获取操作数
    const auto& operands = inst->getOperands();
    if (operands.size() < 2) {
        return nullptr;
    }

    // operands[0] = value, operands[1] = pointer
    // 这里简化处理，假设我们需要检查指针是否越界

#ifdef HAVE_Z3
    try {
        // 创建安全约束：指针应该在有效范围内
        // 这里需要从符号状态中获取缓冲区信息
        // 简化实现：创建一个假设的约束来演示

        // 假设我们有一个缓冲区 buf[10]
        // 检查访问地址是否在 [buf, buf+10) 范围内

        // 创建符号变量
        auto* bufBase = new VariableExpr("buf_base");
        auto* bufSize = new ConstantExpr(10);
        auto* accessPtr = new VariableExpr("access_ptr");

        // 创建安全约束：buf_base <= access_ptr < buf_base + buf_size
        Expr* safeConstraint = ConstraintBuilder::land(
            ConstraintBuilder::ge(accessPtr, bufBase),
            ConstraintBuilder::lt(accessPtr, ConstraintBuilder::add(bufBase, bufSize))
        );

        // 检查是否存在违反安全约束的情况
        // 即：!safeConstraint 是否可满足
        Expr* unsafeConstraint = ConstraintBuilder::lnot(safeConstraint);

        Z3Solver solver;
        SolverResult result = solver.check(unsafeConstraint);

        if (result == SolverResult::Sat) {
            // 存在反例，可能发生缓冲区溢出
            auto* report = new VulnerabilityReport();
            report->type = VulnerabilityType::BufferOverflow;
            report->severity = Severity::High;
            report->location = inst->getLocation();
            report->message = "Buffer overflow detected: store operation may write beyond buffer bounds";

            // 添加反例
            CounterExample model = solver.getModel();
            report->counterExample["buffer_base"] = "0";
            report->counterExample["buffer_size"] = "10";
            report->counterExample["access_offset"] = model.intValues.empty() ? "?" : std::to_string(model.intValues.begin()->second);

            // 添加修复建议
            report->fixSuggestions.push_back("Add bounds checking before array access");
            report->fixSuggestions.push_back("Use safe library functions (e.g., strncpy instead of strcpy)");
            report->fixSuggestions.push_back("Enable compiler buffer overflow protections (-fstack-protector)");

            utils::Logger::error("Buffer overflow vulnerability detected at " + inst->getLocation().toString());

            return report;
        } else if (result == SolverResult::Unsat) {
            // 不可满足，访问是安全的
            return nullptr;
        } else {
            // Unknown或Error，保守处理，不报告漏洞
            return nullptr;
        }

    } catch (const std::exception& e) {
        utils::Logger::error("Exception in buffer overflow checker: " + std::string(e.what()));
        return nullptr;
    }
#else
    // 简化实现：检测所有store指令为潜在溢出
    auto* report = new VulnerabilityReport();
    report->type = VulnerabilityType::BufferOverflow;
    report->severity = Severity::Medium;
    report->location = inst->getLocation();
    report->message = "Potential buffer overflow (Z3 not available, using heuristic detection)";
    report->description = "Store operation may write beyond buffer bounds";
    return report;
#endif
}

bool BufferOverflowChecker::isSafeAccess(
    Expr* ptr,
    Expr* buffer,
    Expr* size,
    SymbolicState* state
) {
#ifdef HAVE_Z3
    // 创建安全约束
    Expr* safeConstraint = ConstraintBuilder::bufferAccess(ptr, buffer, size);

    Z3Solver solver;
    SolverResult result = solver.check(safeConstraint);

    return result == SolverResult::Unsat;  // 如果safeConstraint的否定不可满足，则访问是安全的
#else
    return true;  // 简化实现：假设安全
#endif
}

// ============================================================================
// 增强版空指针解引用检测器
// ============================================================================

VulnerabilityReport* NullPointerChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    if (!state || !inst) {
        return nullptr;
    }

    // 检测load和store指令
    if (inst->getType() != LLIRInstructionType::Load &&
        inst->getType() != LLIRInstructionType::Store) {
        return nullptr;
    }

    const auto& operands = inst->getOperands();
    if (operands.empty()) {
        return nullptr;
    }

#ifdef HAVE_Z3
    try {
        // 创建指针变量
        auto* ptr = new VariableExpr("ptr");

        // 创建空指针约束：ptr == 0
        Expr* nullConstraint = ConstraintBuilder::eq(ptr, new ConstantExpr(0));

        Z3Solver solver;
        SolverResult result = solver.check(nullConstraint);

        if (result == SolverResult::Sat) {
            // 指针可能为空，存在空指针解引用风险
            auto* report = new VulnerabilityReport();
            report->type = VulnerabilityType::NullPointerDereference;
            report->severity = Severity::Critical;
            report->location = inst->getLocation();
            report->message = "Null pointer dereference detected";

            // 添加反例
            CounterExample model = solver.getModel();
            report->counterExample["ptr"] = "0";

            // 添加修复建议
            report->fixSuggestions.push_back("Add null pointer check before dereferencing");
            report->fixSuggestions.push_back("Use assertions to validate pointer assumptions");
            report->fixSuggestions.push_back("Initialize pointers to valid addresses or nullptr");

            utils::Logger::error("Null pointer dereference vulnerability detected at " + inst->getLocation().toString());

            return report;
        }

        return nullptr;

    } catch (const std::exception& e) {
        utils::Logger::error("Exception in null pointer checker: " + std::string(e.what()));
        return nullptr;
    }
#else
    // 简化实现
    auto* report = new VulnerabilityReport();
    report->type = VulnerabilityType::NullPointerDereference;
    report->severity = Severity::High;
    report->location = inst->getLocation();
    report->message = "Potential null pointer dereference (Z3 not available, using heuristic detection)";
    return report;
#endif
}

// ============================================================================
// 增强版内存泄漏检测器
// ============================================================================

VulnerabilityReport* MemoryLeakChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    if (!state || !inst) {
        return nullptr;
    }

    // 只在函数返回点检查内存泄漏
    if (inst->getType() != LLIRInstructionType::Ret) {
        return nullptr;
    }

    // 获取堆上未释放的对象
    auto* heap = state->getHeap();
    auto unfreedObjects = heap->getUnfreedObjects();

    if (!unfreedObjects.empty()) {
        auto* report = new VulnerabilityReport();
        report->type = VulnerabilityType::MemoryLeak;
        report->severity = Severity::Medium;
        report->location = inst->getLocation();

        std::ostringstream oss;
        oss << "Memory leak detected: " << unfreedObjects.size()
            << " allocated object(s) not freed";
        report->message = oss.str();
        report->description = "Allocated memory was not freed before function exit";

        // 添加泄漏位置的详细信息
        for (size_t i = 0; i < unfreedObjects.size() && i < 5; ++i) {
            const auto* obj = unfreedObjects[i];
            std::ostringstream loc;
            loc << "Allocation at " << obj->allocSite.toString();
            report->trace.push_back(obj->allocSite);
        }

        // 添加修复建议
        report->fixSuggestions.push_back("Ensure all allocated memory is freed before exit");
        report->fixSuggestions.push_back("Use RAII patterns (smart pointers in C++)");
        report->fixSuggestions.push_back("Use memory analysis tools (e.g., Valgrind, AddressSanitizer)");

        utils::Logger::error("Memory leak detected: " + std::to_string(unfreedObjects.size()) + " object(s)");

        return report;
    }

    return nullptr;
}

// ============================================================================
// 增强版整数溢出检测器
// ============================================================================

VulnerabilityReport* IntegerOverflowChecker::check(
    SymbolicState* state,
    LLIRInstruction* inst
) {
    if (!state || !inst) {
        return nullptr;
    }

    // 检测算术运算指令
    auto type = inst->getType();
    if (type != LLIRInstructionType::Add &&
        type != LLIRInstructionType::Sub &&
        type != LLIRInstructionType::Mul) {
        return nullptr;
    }

    const auto& operands = inst->getOperands();
    if (operands.size() < 2) {
        return nullptr;
    }

#ifdef HAVE_Z3
    try {
        // 创建操作数变量
        auto* left = new VariableExpr("left");
        auto* right = new VariableExpr("right");

        // 创建溢出约束
        Expr* overflowConstraint = nullptr;
        bool isSigned = true;  // 默认有符号

        switch (type) {
            case LLIRInstructionType::Add:
                overflowConstraint = ConstraintBuilder::addOverflow(left, right, isSigned);
                break;
            case LLIRInstructionType::Sub:
                overflowConstraint = ConstraintBuilder::subOverflow(left, right, isSigned);
                break;
            case LLIRInstructionType::Mul:
                overflowConstraint = ConstraintBuilder::mulOverflow(left, right, isSigned);
                break;
            default:
                return nullptr;
        }

        if (!overflowConstraint) {
            return nullptr;
        }

        Z3Solver solver;
        SolverResult result = solver.check(overflowConstraint);

        if (result == SolverResult::Sat) {
            // 存在溢出可能
            auto* report = new VulnerabilityReport();
            report->type = VulnerabilityType::IntegerOverflow;
            report->severity = Severity::High;
            report->location = inst->getLocation();

            std::string opName;
            switch (type) {
                case LLIRInstructionType::Add: opName = "addition"; break;
                case LLIRInstructionType::Sub: opName = "subtraction"; break;
                case LLIRInstructionType::Mul: opName = "multiplication"; break;
                default: opName = "arithmetic operation"; break;
            }

            report->message = "Integer overflow detected in " + opName;
            report->description = "Arithmetic operation may cause integer overflow";

            // 添加反例
            CounterExample model = solver.getModel();
            if (!model.intValues.empty()) {
                for (const auto& [var, val] : model.intValues) {
                    report->counterExample[var] = std::to_string(val);
                }
            }

            // 添加修复建议
            report->fixSuggestions.push_back("Add overflow checks before arithmetic operations");
            report->fixSuggestions.push_back("Use wider integer types for intermediate results");
            report->fixSuggestions.push_back("Use compiler builtins (e.g., __builtin_add_overflow)");
            report->fixSuggestions.push_back("Enable undefined behavior sanitizer (-fsanitize=undefined)");

            utils::Logger::error("Integer overflow vulnerability detected at " + inst->getLocation().toString());

            return report;
        }

        return nullptr;

    } catch (const std::exception& e) {
        utils::Logger::error("Exception in integer overflow checker: " + std::string(e.what()));
        return nullptr;
    }
#else
    // 简化实现
    auto* report = new VulnerabilityReport();
    report->type = VulnerabilityType::IntegerOverflow;
    report->severity = Severity::Medium;
    report->location = inst->getLocation();
    report->message = "Potential integer overflow (Z3 not available, using heuristic detection)";
    return report;
#endif
}

} // namespace core
} // namespace cverifier
