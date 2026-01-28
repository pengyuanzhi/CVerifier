/**
 * @file PathConstraintSolver.cpp
 * @brief PathConstraint 求解器实现（依赖于 Z3Solver）
 *
 * 这个文件专门用于实现 PathConstraint 中依赖于 SMT 求解器的方法
 * 放在 analyzer 库中避免 core 库依赖 analyzer 库
 */

#include "cverifier/SymbolicState.h"
#include "cverifier/Z3Solver.h"
#include "cverifier/Utils.h"

using cverifier::utils::Logger;

namespace cverifier {
namespace core {

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

} // namespace core
} // namespace cverifier
