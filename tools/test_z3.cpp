/**
 * @file test_z3.cpp
 * @brief Z3求解器测试程序
 */

#include "cverifier/Z3Solver.h"
#include "cverifier/SymbolicState.h"
#include "cverifier/Utils.h"
#include <iostream>

using namespace cverifier;
using namespace cverifier::core;

/**
 * @brief 测试1：简单约束求解
 */
void testSimpleConstraints() {
    std::cout << "=== Test 1: Simple Constraints ===" << std::endl;

#ifdef HAVE_Z3
    Z3Solver solver;

    // 创建简单约束: x > 5 && x < 10
    auto* x = new VariableExpr("x");
    auto* five = new ConstantExpr(5);
    auto* ten = new ConstantExpr(10);

    auto* constraint = ConstraintBuilder::land(
        ConstraintBuilder::gt(x, five),
        ConstraintBuilder::lt(x, ten)
    );

    SolverResult result = solver.check(constraint);

    std::cout << "Constraint: x > 5 && x < 10" << std::endl;
    std::cout << "Result: ";
    switch (result) {
        case SolverResult::Sat:
            std::cout << "SAT (Satisfiable)" << std::endl;
            std::cout << solver.getModel().toString() << std::endl;
            break;
        case SolverResult::Unsat:
            std::cout << "UNSAT (Unsatisfiable)" << std::endl;
            break;
        case SolverResult::Unknown:
            std::cout << "UNKNOWN" << std::endl;
            break;
        case SolverResult::Error:
            std::cout << "ERROR" << std::endl;
            break;
    }
#else
    std::cout << "Z3 not available, skipping test" << std::endl;
#endif
    std::cout << std::endl;
}

/**
 * @brief 测试2：路径约束求解
 */
void testPathConstraints() {
    std::cout << "=== Test 2: Path Constraints ===" << std::endl;

    // 创建路径约束
    PathConstraint pathConstraints;

    // 添加约束: x > 0 && y > 0 && x + y < 10
    auto* x = new VariableExpr("x");
    auto* y = new VariableExpr("y");
    auto* zero = new ConstantExpr(0);
    auto* ten = new ConstantExpr(10);

    pathConstraints.add(ConstraintBuilder::gt(x, zero));
    pathConstraints.add(ConstraintBuilder::gt(y, zero));
    pathConstraints.add(ConstraintBuilder::lt(ConstraintBuilder::add(x, y), ten));

    std::cout << "Path constraints: x > 0 && y > 0 && x + y < 10" << std::endl;
    std::cout << "Constraints: " << pathConstraints.toString() << std::endl;

    bool satisfiable = pathConstraints.isSatisfiable();
    std::cout << "Satisfiable: " << (satisfiable ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

/**
 * @brief 测试3：缓冲区溢出检测
 */
void testBufferOverflow() {
    std::cout << "=== Test 3: Buffer Overflow Detection ===" << std::endl;

#ifdef HAVE_Z3
    Z3Solver solver;

    // 创建缓冲区访问场景
    // 假设有缓冲区 buf[10]，访问 buf[index]
    auto* bufBase = new ConstantExpr(1000);  // 缓冲区基地址
    auto* bufSize = new ConstantExpr(10);    // 缓冲区大小
    auto* index = new VariableExpr("index"); // 索引变量

    // 创建安全约束: 0 <= index < 10
    Expr* safeConstraint = ConstraintBuilder::land(
        ConstraintBuilder::ge(index, new ConstantExpr(0)),
        ConstraintBuilder::lt(index, bufSize)
    );

    // 测试安全访问
    std::cout << "Test 3a: Safe access (index = 5)" << std::endl;
    auto* safeIndex = new ConstantExpr(5);
    Expr* safeAccess = ConstraintBuilder::land(
        ConstraintBuilder::ge(safeIndex, new ConstantExpr(0)),
        ConstraintBuilder::lt(safeIndex, bufSize)
    );

    SolverResult safeResult = solver.check(safeAccess);
    std::cout << "Safe access result: "
              << (safeResult == SolverResult::Sat ? "SAT" : "UNSAT") << std::endl;

    // 测试不安全访问
    std::cout << "\nTest 3b: Unsafe access (index = 15)" << std::endl;
    auto* unsafeIndex = new ConstantExpr(15);
    Expr* unsafeAccess = ConstraintBuilder::land(
        ConstraintBuilder::ge(unsafeIndex, new ConstantExpr(0)),
        ConstraintBuilder::lt(unsafeIndex, bufSize)
    );

    SolverResult unsafeResult = solver.check(unsafeAccess);
    std::cout << "Unsafe access result: "
              << (unsafeResult == SolverResult::Sat ? "SAT" : "UNSAT") << std::endl;

    if (unsafeResult == SolverResult::Unsat) {
        std::cout << "Buffer overflow detected!" << std::endl;
    }
#else
    std::cout << "Z3 not available, skipping test" << std::endl;
#endif
    std::cout << std::endl;
}

/**
 * @brief 测试4：空指针检测
 */
void testNullPointer() {
    std::cout << "=== Test 4: Null Pointer Detection ===" << std::endl;

#ifdef HAVE_Z3
    Z3Solver solver;

    // 创建指针变量
    auto* ptr = new VariableExpr("ptr");

    // 测试指针可能为null
    std::cout << "Test 4a: Can ptr be NULL?" << std::endl;
    Expr* nullCheck = ConstraintBuilder::eq(ptr, new ConstantExpr(0));

    SolverResult result = solver.check(nullCheck);
    std::cout << "Result: "
              << (result == SolverResult::Sat ? "YES (possible null)" : "NO (cannot be null)")
              << std::endl;

    if (result == SolverResult::Sat) {
        std::cout << "Null pointer dereference risk detected!" << std::endl;
        std::cout << solver.getModel().toString() << std::endl;
    }
#else
    std::cout << "Z3 not available, skipping test" << std::endl;
#endif
    std::cout << std::endl;
}

/**
 * @brief 测试5：整数溢出检测
 */
void testIntegerOverflow() {
    std::cout << "=== Test 5: Integer Overflow Detection ===" << std::endl;

#ifdef HAVE_Z3
    Z3Solver solver;

    // 测试加法溢出（无符号）
    auto* a = new VariableExpr("a");
    auto* b = new VariableExpr("b");

    // 检查 a + b 是否可能溢出
    // 对于无符号：a + b < a 表示溢出
    std::cout << "Test 5a: Can a + b overflow (unsigned)?" << std::endl;
    Expr* overflowCheck = ConstraintBuilder::addOverflow(a, b, false);

    // 设置范围使溢出可能发生
    solver.push();
    solver.addAssertion(ConstraintBuilder::ge(a, new ConstantExpr(INT_MAX - 10)));
    solver.addAssertion(ConstraintBuilder::ge(b, new ConstantExpr(20)));

    SolverResult result = solver.check(overflowCheck);
    std::cout << "Overflow possible: "
              << (result == SolverResult::Sat ? "YES" : "NO") << std::endl;

    solver.pop();
#else
    std::cout << "Z3 not available, skipping test" << std::endl;
#endif
    std::cout << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "CVerifier Z3 Solver Test Suite" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << std::endl;

    // 设置日志级别
    utils::Logger::setLevel(utils::Logger::Level::Info);

#ifdef HAVE_Z3
    unsigned int major, minor, build, revision;
    Z3_get_version(&major, &minor, &build, &revision);
    std::cout << "Z3 Version: " << major << "." << minor << "." << build << "." << revision << std::endl;
    std::cout << std::endl;
#else
    std::cout << "Z3: Not Available (tests will use simplified implementation)" << std::endl;
    std::cout << std::endl;
#endif

    // 运行测试
    testSimpleConstraints();
    testPathConstraints();
    testBufferOverflow();
    testNullPointer();
    testIntegerOverflow();

    std::cout << "===============================" << std::endl;
    std::cout << "All tests completed!" << std::endl;

    return 0;
}
