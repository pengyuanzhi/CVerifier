#ifndef CVERIFIER_ANALYZER_H
#define CVERIFIER_ANALYZER_H

#include "cverifier/Core.h"
#include "cverifier/SymbolicState.h"
#include "cverifier/LLIRModule.h"
#include <memory>
#include <string>
#include <vector>

namespace cverifier {

// ============================================================================
// 分析器接口
// ============================================================================

/**
 * @brief SMT求解器接口
 */
class ISMTSolver {
public:
    virtual ~ISMTSolver() = default;

    /**
     * @brief 检查约束的可满足性
     * @param constraints 约束列表
     * @return true=可满足(SAT), false=不可满足(UNSAT)
     */
    virtual bool check(const std::vector<Expr*>& constraints) = 0;

    /**
     * @brief 获取模型（反例）
     */
    virtual std::unordered_map<std::string, int64_t> getModel() = 0;

    /**
     * @brief 推入约束上下文
     */
    virtual void push() = 0;

    /**
     * @brief 弹出约束上下文
     */
    virtual void pop() = 0;
};

/**
 * @brief Z3求解器实现
 */
class Z3Solver : public ISMTSolver {
public:
    Z3Solver();
    ~Z3Solver() override;

    bool check(const std::vector<Expr*>& constraints) override;
    std::unordered_map<std::string, int64_t> getModel() override;
    void push() override;
    void pop() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// 符号执行引擎
// ============================================================================

/**
 * @brief 路径探索策略
 */
enum class ExplorationStrategy {
    DFS,    ///< 深度优先
    BFS,    ///< 广度优先
    Hybrid, ///< 混合策略
    Random  ///< 随机策略
};

/**
 * @brief 符号执行引擎接口
 */
class ISymbolicExecutionEngine {
public:
    virtual ~ISymbolicExecutionEngine() = default;

    /**
     * @brief 分析函数
     * @param function 要分析的函数
     * @param entryState 初始状态
     * @return 分析结果
     */
    virtual AnalysisResult analyze(
        LLIRFunction* function,
        SymbolicState* entryState = nullptr
    ) = 0;

    /**
     * @brief 设置探索策略
     */
    virtual void setStrategy(ExplorationStrategy strategy) = 0;

    /**
     * @brief 设置最大探索深度
     */
    virtual void setMaxDepth(int depth) = 0;

    /**
     * @brief 停止分析
     */
    virtual void stop() = 0;
};

/**
 * @brief 默认符号执行引擎实现
 */
class SymbolicExecutionEngine : public ISymbolicExecutionEngine {
public:
    SymbolicExecutionEngine(std::unique_ptr<ISMTSolver> solver);
    ~SymbolicExecutionEngine() override;

    AnalysisResult analyze(
        LLIRFunction* function,
        SymbolicState* entryState = nullptr
    ) override;

    void setStrategy(ExplorationStrategy strategy) override;
    void setMaxDepth(int depth) override;
    void stop() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// 抽象解释器
// ============================================================================

/**
 * @brief 抽象域类型
 */
enum class AbstractDomainType {
    Constant,   ///< 常量域
    Interval,   ///< 区间域
    Octagon,    ///< 八边形域
    Polyhedra   ///< 多面体域
};

/**
 * @brief 抽象解释器接口
 */
class IAbstractInterpreter {
public:
    virtual ~IAbstractInterpreter() = default;

    /**
     * @brief 计算不动点
     * @param function 要分析的函数
     * @return 分析结果
     */
    virtual AnalysisResult computeFixpoint(LLIRFunction* function) = 0;

    /**
     * @brief 设置抽象域类型
     */
    virtual void setDomain(AbstractDomainType domain) = 0;
};

/**
 * @brief 区间抽象解释器
 */
class IntervalAbstractInterpreter : public IAbstractInterpreter {
public:
    IntervalAbstractInterpreter();
    ~IntervalAbstractInterpreter() override;

    AnalysisResult computeFixpoint(LLIRFunction* function) override;
    void setDomain(AbstractDomainType domain) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// 漏洞检测器
// ============================================================================

/**
 * @brief 漏洞检测器接口
 */
class IVulnerabilityChecker {
public:
    virtual ~IVulnerabilityChecker() = default;

    /**
     * @brief 检查漏洞
     * @param function 要检查的函数
     * @return 发现的漏洞列表
     */
    virtual std::vector<VulnerabilityReport> check(LLIRFunction* function) = 0;

    /**
     * @brief 获取检测器名称
     */
    virtual std::string getName() const = 0;
};

/**
 * @brief 缓冲区溢出检测器
 */
class BufferOverflowChecker : public IVulnerabilityChecker {
public:
    BufferOverflowChecker(std::unique_ptr<ISMTSolver> solver);
    ~BufferOverflowChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "BufferOverflow"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 空指针解引用检测器
 */
class NullPointerChecker : public IVulnerabilityChecker {
public:
    NullPointerChecker(std::unique_ptr<ISMTSolver> solver);
    ~NullPointerChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "NullPointer"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 内存泄漏检测器
 */
class MemoryLeakChecker : public IVulnerabilityChecker {
public:
    MemoryLeakChecker();
    ~MemoryLeakChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "MemoryLeak"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 整数溢出检测器
 */
class IntegerOverflowChecker : public IVulnerabilityChecker {
public:
    IntegerOverflowChecker(std::unique_ptr<ISMTSolver> solver);
    ~IntegerOverflowChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "IntegerOverflow"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// 分析管理器
// ============================================================================

/**
 * @brief 分析配置
 */
struct AnalysisConfig {
    AnalysisOptions options;                          ///< 基本选项
    ExplorationStrategy strategy = ExplorationStrategy::Hybrid;  ///< 探索策略
    AbstractDomainType domain = AbstractDomainType::Interval;    ///< 抽象域

    bool enableBufferOverflowCheck = true;   ///< 启用缓冲区溢出检测
    bool enableNullPointerCheck = true;      ///< 启用空指针检测
    bool enableMemoryLeakCheck = true;       ///< 启用内存泄漏检测
    bool enableIntegerOverflowCheck = true;  ///< 启用整数溢出检测
};

/**
 * @brief 分析管理器（整合所有分析组件）
 */
class AnalysisManager {
public:
    AnalysisManager();
    ~AnalysisManager();

    /**
     * @brief 分析C源文件
     * @param sourceFile 源文件路径
     * @param config 分析配置
     * @return 分析结果
     */
    AnalysisResult analyze(
        const std::string& sourceFile,
        const AnalysisConfig& config = AnalysisConfig()
    );

    /**
     * @brief 分析LLIR模块
     * @param module LLIR模块
     * @param config 分析配置
     * @return 分析结果
     */
    AnalysisResult analyze(
        LLIRModule* module,
        const AnalysisConfig& config = AnalysisConfig()
    );

    /**
     * @brief 设置解析器
     */
    void setParser(std::unique_ptr<IParser> parser);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace cverifier

#endif // CVERIFIER_ANALYZER_H
