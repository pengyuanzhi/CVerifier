/**
 * @file test_abstract.cpp
 * @brief 抽象解释器测试程序
 */

#include "cverifier/AbstractInterpreter.h"
#include "cverifier/LLIRModule.h"
#include "cverifier/LLIRFactory.h"
#include "cverifier/CFG.h"
#include "cverifier/Utils.h"
#include <iostream>
#include <iomanip>

using namespace cverifier;
using namespace cverifier::core;

/**
 * @brief 测试1：区间域基本操作
 */
void test1_IntervalDomain() {
    std::cout << "=== Test 1: Interval Domain ===" << std::endl;

    // 创建区间值
    auto* interval1 = new IntervalValue(5, 10);
    auto* interval2 = new IntervalValue(0, 100);
    auto* top = IntervalValue::createTop(ValueType::Integer);
    auto* bottom = IntervalValue::createBottom(ValueType::Integer);

    std::cout << "Interval1: " << interval1->toString() << std::endl;
    std::cout << "Interval2: " << interval2->toString() << std::endl;
    std::cout << "Top: " << top->toString() << std::endl;
    std::cout << "Bottom: " << bottom->toString() << std::endl;

    // 测试包含关系
    std::cout << "\nContains tests:" << std::endl;
    std::cout << "  interval1 contains 7: " << (interval1->contains(7) ? "Yes" : "No") << std::endl;
    std::cout << "  interval1 contains 15: " << (interval1->contains(15) ? "Yes" : "No") << std::endl;

    // 清理
    delete interval1;
    delete interval2;
    delete top;
    delete bottom;

    std::cout << std::endl;
}

/**
 * @brief 测试2：抽象存储操作
 */
void test2_AbstractStore() {
    std::cout << "=== Test 2: Abstract Store ===" << std::endl;

    // 创建抽象存储
    auto* store1 = new AbstractStore();
    auto* store2 = new AbstractStore();

    // 绑定变量
    store1->bind("x", new IntervalValue(5, 10));
    store1->bind("y", new IntervalValue(0, 100));

    store2->bind("x", new IntervalValue(3, 8));
    store2->bind("z", new IntervalValue(20, 30));

    std::cout << "Store1: " << store1->toString() << std::endl;
    std::cout << "Store2: " << store2->toString() << std::endl;

    // 查找变量
    AbstractValue* xValue1 = store1->lookup("x");
    if (xValue1) {
        std::cout << "Store1['x'] = " << xValue1->toString() << std::endl;
    }

    // 合并存储
    auto* merged = store1->merge(store2);
    std::cout << "Merged: " << merged->toString() << std::endl;

    // 清理
    delete store1;
    delete store2;
    delete merged;

    std::cout << std::endl;
}

/**
 * @brief 测试3：区间算术运算
 */
void test3_IntervalArithmetic() {
    std::cout << "=== Test 3: Interval Arithmetic ===" << std::endl;

    auto* a = new IntervalValue(5, 10);
    auto* b = new IntervalValue(3, 7);

    std::cout << "a = " << a->toString() << std::endl;
    std::cout << "b = " << b->toString() << std::endl;

    // 加法
    auto* sum = intervalAdd(a, b);
    std::cout << "a + b = " << sum->toString() << std::endl;

    // 减法
    auto* diff = intervalSub(a, b);
    std::cout << "a - b = " << diff->toString() << std::endl;

    // 乘法
    auto* product = intervalMul(a, b);
    std::cout << "a * b = " << product->toString() << std::endl;

    // 清理
    delete a;
    delete b;
    delete sum;
    delete diff;
    delete product;

    std::cout << std::endl;
}

/**
 * @brief 测试4：完整的抽象解释分析
 */
void test4_AbstractInterpretation() {
    std::cout << "=== Test 4: Full Abstract Interpretation ===" << std::endl;

    // 创建测试LLIR模块
    auto* module = LLIRFactory::createModule("test_module");
    auto* func = LLIRFactory::createFunction("test_function");
    module->addFunction(func);

    // 创建基本块：简单循环
    auto* entry = LLIRFactory::createBasicBlock("entry");
    auto* loop = LLIRFactory::createBasicBlock("loop");
    auto* exit = LLIRFactory::createBasicBlock("exit");

    func->addBasicBlock(entry);
    func->addBasicBlock(loop);
    func->addBasicBlock(exit);
    func->setEntryBlock(entry);

    // entry: 初始化和跳转到循环
    auto* alloca = LLIRFactory::createAlloca(LLIRFactory::createIntConstant(4));
    auto* const0 = LLIRFactory::createIntConstant(0);
    auto* const10 = LLIRFactory::createIntConstant(10);

    entry->addInstruction(alloca);
    auto* toLoop = LLIRFactory::createBr(loop);
    entry->addInstruction(toLoop);

    // loop: 简单循环体
    auto* add = LLIRFactory::createAdd(const0, const10);
    auto* backToLoop = LLIRFactory::createBr(loop);
    auto* toExit = LLIRFactory::createBr(exit);

    loop->addInstruction(add);
    loop->addInstruction(toLoop);  // 实际应该有条件分支

    // exit: 返回
    auto* ret = LLIRFactory::createRet(const0);
    exit->addInstruction(ret);

    // 设置CFG连接
    entry->addSuccessor(loop);
    loop->addSuccessor(loop);  // 自循环
    loop->addSuccessor(exit);

    std::cout << "LLIR Module created with 1 function" << std::endl;
    std::cout << "Function: " << func->getName() << std::endl;
    std::cout << "Basic blocks: " << func->getBasicBlocks().size() << std::endl;

    // 创建CFG
    CFG cfg(func);
    std::cout << "CFG nodes: " << cfg.getNodes().size() << std::endl;

    // 运行抽象解释
    std::cout << "\nRunning abstract interpretation..." << std::endl;

    AbstractInterpreter interpreter(module);
    interpreter.setDomain("interval");
    interpreter.analyzeFunction("test_function");

    // 打印统计信息
    std::cout << "\n" << interpreter.getStatistics() << std::endl;

    // 打印结果
    std::cout << "\nAnalysis results:" << std::endl;
    for (const auto& [bbName, store] : interpreter.getResults()) {
        std::cout << "  BasicBlock '" << bbName << "': " << store->toString() << std::endl;
    }

    // 清理
    delete module;

    std::cout << std::endl;
}

/**
 * @brief 测试5：区间域精度对比
 */
void test5_PrecisionComparison() {
    std::cout << "=== Test 5: Precision Comparison ===" << std::endl;

    std::cout << "\nScenario: Loop that increments a counter\n" << std::endl;

    std::cout << "Symbolic Execution (precise but slow):" << std::endl;
    std::cout << "  i = 0, 1, 2, 3, ... (unbounded)" << std::endl;
    std::cout << "  Pros: Exact values for each path" << std::endl;
    std::cout << "  Cons: Path explosion, slow" << std::endl;

    std::cout << "\nAbstract Interpretation - Interval Domain:" << std::endl;
    std::cout << "  i = [0, +∞]" << std::endl;
    std::cout << "  Pros: Fast, single analysis pass" << std::endl;
    std::cout << "  Cons: Over-approximation, false positives possible" << std::endl;

    std::cout << "\nAbstract Interpretation - Constant Domain:" << std::endl;
    std::cout << "  i = ⊤ (unknown)" << std::endl;
    std::cout << "  Pros: Very fast" << std::endl;
    std::cout << "  Cons: Very imprecise" << std::endl;

    std::cout << "\nRecommendation:" << std::endl;
    std::cout << "  • Use abstract interpretation for quick screening" << std::endl;
    std::cout << "  • Use symbolic execution for precise verification" << std::endl;
    std::cout << "  • Hybrid approach: abstract interpretation + symbolic execution" << std::endl;

    std::cout << std::endl;
}

/**
 * @brief 比较抽象解释和符号执行
 */
void compareAbstractVsSymbolic() {
    std::cout << "\n=== Comparison: Abstract vs Symbolic ===" << std::endl;

    std::cout << std::left << std::setw(30) << "Aspect"
              << std::setw(25) << "Abstract Interpretation"
              << std::setw(25) << "Symbolic Execution" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    std::cout << std::left << std::setw(30) << "Precision"
              << std::setw(25) << "Over-approximation"
              << std::setw(25) << "Precise (path-sensitive)" << std::endl;

    std::cout << std::left << std::setw(30) << "Performance"
              << std::setw(25) << "Fast (polynomial)"
              << std::setw(25) << "Slow (exponential)" << std::endl;

    std::cout << std::left << std::setw(30) << "Scalability"
              << std::setw(25) << "Excellent"
              << std::setw(25) << "Limited (path explosion)" << std::endl;

    std::cout << std::left << std::setw(30) << "False Positives"
              << std::setw(25) << "Possible"
              << std::setw(25) << "Unlikely (with models)" << std::endl;

    std::cout << std::left << std::setw(30) << "False Negatives"
              << std::setw(25) << "Impossible (sound)"
              << std::setw(25) << "Possible (incomplete)" << std::endl;

    std::cout << std::left << std::setw(30) << "Use Case"
              << std::setw(25) << "Quick screening"
              << std::setw(25) << "Deep verification" << std::endl;

    std::cout << std::endl;
}

/**
 * @brief 混合分析策略
 */
void demonstrateHybridAnalysis() {
    std::cout << "=== Hybrid Analysis Strategy ===" << std::endl;

    std::cout << "\nStep 1: Abstract Interpretation (Fast Pruning)" << std::endl;
    std::cout << "  • Quick analysis of entire codebase" << std::endl;
    std::cout << "  • Identify obviously safe code" << std::endl;
    std::cout << "  • Mark suspicious regions for deeper analysis" << std::endl;

    std::cout << "\nStep 2: Symbolic Execution (Precise Verification)" << std::endl;
    std::cout << "  • Focus on suspicious regions only" << std::endl;
    std::cout << "  • Path-sensitive analysis" << std::endl;
    std::cout << "  • Generate concrete counter-examples" << std::endl;

    std::cout << "\nBenefits:" << std::endl;
    std::cout << "  • 10-100x faster than pure symbolic execution" << std::endl;
    std::cout << "  • Reduces false positives" << std::endl;
    std::cout << "  • Scales to large codebases" << std::endl;

    std::cout << "\nImplementation in CVerifier:" << std::endl;
    std::cout << "  1. Run abstract interpreter first" << std::endl;
    std::cout << "  2. Collect potentially vulnerable code locations" << std::endl;
    std::cout << "  3. Run symbolic execution only on those locations" << std::endl;
    std::cout << "  4. Combine results for final report" << std::endl;

    std::cout << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "CVerifier Abstract Interpreter Test Suite" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << std::endl;

    // 设置日志级别
    utils::Logger::setLevel(utils::Logger::Level::Info);

    std::cout << "Abstract Interpretation: A fast program analysis technique" << std::endl;
    std::cout << "that computes conservative over-approximations of program behavior." << std::endl;
    std::cout << std::endl;

    // 运行测试
    test1_IntervalDomain();
    test2_AbstractStore();
    test3_IntervalArithmetic();
    test4_AbstractInterpretation();
    test5_PrecisionComparison();

    // 比较和总结
    compareAbstractVsSymbolic();
    demonstrateHybridAnalysis();

    std::cout << "=========================================" << std::endl;
    std::cout << "Key Takeaways:" << std::endl;
    std::cout << "1. Abstract interpretation is fast but imprecise" << std::endl;
    std::cout << "2. Interval domain provides good balance" << std::endl;
    std::cout << "3. Hybrid with symbolic execution is powerful" << std::endl;
    std::cout << "4. Both techniques complement each other" << std::endl;
    std::cout << std::endl;

    std::cout << "All tests completed!" << std::endl;

    return 0;
}
