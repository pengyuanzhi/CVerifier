# CVerifier 详细设计文档

| 文档版本 | 日期 | 作者 | 变更说明 |
|---------|------|------|---------|
| 0.1.0 | 2025-01-28 | CVerifier Team | 初始版本 |

---

## 目录

- [1. 模块设计](#1-模块设计)
  - [1.1 前端模块](#11-前端模块)
  - [1.2 核心模块](#12-核心模块)
  - [1.3 分析器模块](#13-分析器模块)
  - [1.4 检测器模块](#14-检测器模块)
  - [1.5 报告模块](#15-报告模块)
- [2. 接口设计](#2-接口设计)
  - [2.1 模块间接口](#21-模块间接口)
  - [2.2 外部接口](#22-外部接口)
- [3. 数据结构设计](#3-数据结构设计)
  - [3.1 LLIR 数据结构](#31-llir-数据结构)
  - [3.2 符号状态数据结构](#32-符号状态数据结构)
  - [3.3 约束表达式数据结构](#33-约束表达式数据结构)
- [4. 算法设计](#4-算法设计)
  - [4.1 符号执行算法](#41-符号执行算法)
  - [4.2 抽象解释算法](#42-抽象解释算法)
  - [4.3 路径探索算法](#43-路径探索算法)
  - [4.4 漏洞检测算法](#44-漏洞检测算法)
- [5. 关键技术方案](#5-关键技术方案)
  - [5.1 状态空间爆炸解决方案](#51-状态空间爆炸解决方案)
  - [5.2 上下文敏感分析方案](#52-上下文敏感分析方案)
  - [5.3 外部函数建模方案](#53-外部函数建模方案)
  - [5.4 性能优化方案](#54-性能优化方案)
- [6. 规约系统设计](#6-规约系统设计-新增) ⭐新增
  - [6.1 C语言验证API设计](#61-c语言验证api设计)
- [7. 插件系统设计](#7-插件系统设计-新增) ⭐新增
- [8. 浮点检测算法设计](#8-浮点检测算法设计-新增) ⭐新增
- [9. Web API 设计](#9-web-api-设计-新增) ⭐新增

---

## 1. 模块设计

### 1.1 前端模块

#### 1.1.1 ClangParser

**文件位置**：`src/frontend/clang/ClangParser.h/cpp`

**职责**：解析 C 源文件，生成 LLIR

**类设计**：
```cpp
class ClangParser : public IParser {
public:
    ClangParser();
    ~ClangParser() override;

    // 解析源文件
    std::unique_ptr<LLIRModule> parse(
        const std::string& sourceFile,
        const ParseOptions& options = ParseOptions()
    ) override;

    // 解析源代码字符串
    std::unique_ptr<LLIRModule> parseString(
        const std::string& sourceCode,
        const ParseOptions& options = ParseOptions()
    ) override;

    std::string getLastError() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
```

**关键方法**：
- `parse()`: 解析文件
- `parseString()`: 解析字符串
- `getLastError()`: 获取错误信息

**依赖**：LLVM/Clang

#### 1.1.2 ASTConsumer

**文件位置**：`src/frontend/clang/ASTConsumer.h/cpp`

**职责**：消费 Clang AST，转换为 LLIR

**类设计**：
```cpp
class AnalysisASTConsumer : public clang::ASTConsumer {
public:
    explicit AnalysisASTConsumer(LLIRModule* module);

    void HandleTranslationUnit(clang::ASTContext& ctx) override;

private:
    LLIRModule* module_;
    std::unique_ptr<IRConverter> converter_;
};
```

#### 1.1.3 IRConverter

**文件位置**：`src/frontend/clang/IRConverter.h/cpp`

**职责**：将 Clang AST 转换为 LLIR

**类设计**：
```cpp
class IRConverter : public clang::StmtVisitor<IRConverter> {
public:
    explicit IRConverter(LLIRModule* module);

    // 访问语句
    void VisitFunctionDecl(clang::FunctionDecl* func);
    void VisitIfStmt(clang::IfStmt* stmt);
    void VisitWhileStmt(clang::WhileStmt* stmt);
    void VisitForStmt(clang::ForStmt* stmt);
    void VisitReturnStmt(clang::ReturnStmt* stmt);
    void VisitBinaryOperator(clang::BinaryOperator* op);
    void VisitDeclRefExpr(clang::DeclRefExpr* expr);
    // ... 更多访问方法

private:
    LLIRModule* module_;
    LLIRFunction* currentFunction_;
    LLIRBasicBlock* currentBlock_;
};
```

### 1.2 核心模块

#### 1.2.1 LLIR 模块

**文件位置**：`src/core/IR/LLIRModule.h/cpp`

**职责**：定义中间表示数据结构

**核心类**：
```cpp
// LLIR 模块
class LLIRModule {
public:
    explicit LLIRModule(const std::string& name = "module");
    ~LLIRModule();

    void addFunction(LLIRFunction* func);
    LLIRFunction* getFunction(const std::string& name) const;
    const std::vector<LLIRFunction*>& getFunctions() const;
    bool validate() const;
    std::string dump() const;

private:
    std::string name_;
    std::vector<LLIRFunction*> functions_;
    std::unordered_map<std::string, LLIRFunction*> funcMap_;
};

// LLIR 函数
class LLIRFunction {
public:
    explicit LLIRFunction(const std::string& name);

    void addBasicBlock(LLIRBasicBlock* bb);
    void setEntryBlock(LLIRBasicBlock* entry);
    LLIRBasicBlock* getEntryBlock() const;
    LLIRBasicBlock* getBasicBlock(const std::string& name) const;

    std::string getName() const;
    const std::vector<LLIRBasicBlock*>& getBasicBlocks() const;

private:
    std::string name_;
    std::vector<LLIRBasicBlock*> basicBlocks_;
    std::unordered_map<std::string, LLIRBasicBlock*> bbMap_;
    LLIRBasicBlock* entryBlock_;
};

// LLIR 基本块
class LLIRBasicBlock {
public:
    explicit LLIRBasicBlock(const std::string& name = "");

    void addInstruction(LLIRInstruction* inst);
    void addSuccessor(LLIRBasicBlock* succ);
    void addPredecessor(LLIRBasicBlock* pred);

    const std::vector<LLIRInstruction*>& getInstructions() const;
    const std::vector<LLIRBasicBlock*>& getSuccessors() const;
    const std::vector<LLIRBasicBlock*>& getPredecessors() const;

    std::string getName() const;

private:
    std::string name_;
    std::vector<LLIRInstruction*> instructions_;
    std::vector<LLIRBasicBlock*> successors_;
    std::vector<LLIRBasicBlock*> predecessors_;
};

// LLIR 指令
class LLIRInstruction : public LLIRValue {
public:
    LLIRInstruction(LLIRInstructionType type, SourceLocation loc = {});

    LLIRInstructionType getType() const;
    SourceLocation getLocation() const;

    void addOperand(LLIRValue* operand);
    const std::vector<LLIRValue*>& getOperands() const;

private:
    LLIRInstructionType type_;
    SourceLocation location_;
    std::vector<LLIRValue*> operands_;
};
```

#### 1.2.2 SymbolicState 模块

**文件位置**：`src/core/State/SymbolicState.h/cpp`

**职责**：管理符号执行状态

**核心类**：
```cpp
// 符号表达式基类
class Expr {
public:
    virtual ~Expr() = default;
    ExprType getType() const { return type_; }
    virtual std::string toString() const = 0;

protected:
    Expr(ExprType type) : type_(type) {}
    ExprType type_;
};

// 常量表达式
class ConstantExpr : public Expr {
public:
    explicit ConstantExpr(int64_t value);
    int64_t getValue() const;
    std::string toString() const override;

private:
    int64_t value_;
};

// 变量表达式
class VariableExpr : public Expr {
public:
    explicit VariableExpr(const std::string& name);
    const std::string& getName() const;
    std::string toString() const override;

private:
    std::string name_;
};

// 二元操作表达式
class BinaryOpExpr : public Expr {
public:
    BinaryOpExpr(BinaryOpType op, Expr* left, Expr* right);

    BinaryOpType getOp() const;
    Expr* getLeft() const;
    Expr* getRight() const;
    std::string toString() const override;

private:
    BinaryOpType op_;
    Expr* left_;
    Expr* right_;
};

// 符号存储
class SymbolicStore {
public:
    void bind(const std::string& var, Expr* expr);
    Expr* lookup(const std::string& var) const;
    std::unique_ptr<SymbolicStore> clone() const;
    void merge(const SymbolicStore& other);

private:
    std::unordered_map<std::string, Expr*> store_;
};

// 符号堆
class SymbolicHeap {
public:
    Expr* allocate(Expr* size, const SourceLocation& loc);
    void free(Expr* address);
    Expr* load(Expr* address, Expr* offset = nullptr);
    void store(Expr* address, Expr* value, Expr* offset = nullptr);
    bool mayBeNull(Expr* address) const;

private:
    std::vector<std::unique_ptr<HeapObject>> objects_;
    std::unordered_map<Expr*, HeapObject*> addressMap_;
};

// 路径约束
class PathConstraint {
public:
    void add(Expr* constraint);
    const std::vector<Expr*>& getConstraints() const;
    bool isSatisfiable() const;
    void simplify();

private:
    std::vector<Expr*> constraints_;
};

// 符号状态
class SymbolicState {
public:
    SymbolicState();
    explicit SymbolicState(SymbolicState* parent);

    SymbolicStore* getStore();
    SymbolicHeap* getHeap();
    PathConstraint* getPathConstraint();

    std::unique_ptr<SymbolicState> clone() const;
    void assign(const std::string& var, Expr* expr);
    Expr* lookup(const std::string& var) const;
    void addConstraint(Expr* constraint);

private:
    SymbolicStore store_;
    SymbolicHeap heap_;
    PathConstraint pathConstraint_;
    SymbolicState* parent_;
};
```

### 1.3 分析器模块

#### 1.3.1 SymbolicExecutionEngine

**文件位置**：`src/analyzer/SymbolicExecution/Engine.h/cpp`

**职责**：执行符号执行

**类设计**：
```cpp
class SymbolicExecutionEngine : public ISymbolicExecutionEngine {
public:
    explicit SymbolicExecutionEngine(std::unique_ptr<ISMTSolver> solver);
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
```

**核心算法**：
```cpp
void SymbolicExecutionEngine::explorePaths(
    SymbolicState* state, LLIRBasicBlock* bb) {

    // 检查深度限制
    if (state->getDepth() > maxDepth_) {
        return;
    }

    // 执行基本块中的每条指令
    for (auto* inst : bb->getInstructions()) {
        executeInstruction(state, inst);
    }

    // 处理后继基本块
    for (auto* succ : bb->getSuccessors()) {
        // 克隆状态
        auto newState = state->clone();

        // 添加分支条件
        if (bb->getTerminator()->isConditional()) {
            auto condition = evaluateBranch(newState, succ);
            newState->addConstraint(condition);
        }

        // 检查可满足性
        if (solver_->check(newState->getConstraints())) {
            worklist_.push(newState, succ);
        }
    }
}
```

#### 1.3.2 AbstractInterpreter

**文件位置**：`src/analyzer/AbstractInterpretation/Interpreter.h/cpp`

**职责**：执行抽象解释

**类设计**：
```cpp
template<typename D>
class AbstractInterpreter {
public:
    using AbstractState = typename D::Type;

    AnalysisResult computeFixpoint(LLIRFunction* function);
    void setDomain(std::unique_ptr<D> domain);

private:
    std::unique_ptr<D> domain_;
    std::map<LLIRBasicBlock*, AbstractState> states_;
};
```

**不动点迭代算法**：
```cpp
void computeFixpoint(ControlFlowGraph* cfg) {
    bool changed = true;
    int iteration = 0;

    while (changed && iteration < MAX_ITERATIONS) {
        changed = false;

        for (auto* bb : cfg->getBasicBlocks()) {
            auto oldState = states_[bb];

            // 计算新状态
            auto newState = analyzeBlock(bb);

            // 合并状态
            if (newState != oldState) {
                states_[bb] = domain_->join(oldState, newState);
                changed = true;
            }
        }

        // 周期性应用 widening
        if (iteration % WIDENING_INTERVAL == 0) {
            applyWidening();
        }

        iteration++;
    }
}
```

### 1.4 检测器模块

#### 1.4.1 BufferOverflowChecker

**文件位置**：`src/analyzer/Checkers/BufferOverflowChecker.h/cpp`

**职责**：检测缓冲区溢出

**类设计**：
```cpp
class BufferOverflowChecker : public IVulnerabilityChecker {
public:
    explicit BufferOverflowChecker(std::unique_ptr<ISMTSolver> solver);
    ~BufferOverflowChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "BufferOverflow"; }

private:
    void checkMemoryAccess(SymbolicState* state, LLIRInstruction* inst);
    void checkArrayAccess(SymbolicState* state, LLIRInstruction* inst);
    void checkPointerArithmetic(SymbolicState* state, LLIRInstruction* inst);

    std::unique_ptr<ISMTSolver> solver_;
};
```

**检测算法**：
```cpp
void BufferOverflowChecker::checkMemoryAccess(
    SymbolicState* state, LLIRInstruction* inst) {

    // 1. 获取访问信息
    Expr* baseAddr = state->lookup(inst->getOperand(0));
    Expr* offset = state->lookup(inst->getOperand(1));
    Expr* accessSize = getAccessSize(inst);

    // 2. 获取缓冲区大小
    Expr* bufferSize = getBufferSize(baseAddr);

    // 3. 构造安全约束
    Expr* safeConstraint = solver_->mkAnd(
        solver_->mkGe(offset, solver_->mkInt(0)),
        solver_->mkLe(
            solver_->mkAdd(offset, accessSize),
            bufferSize
        )
    );

    // 4. 检查约束
    auto constraints = state->getConstraints();
    constraints.push_back(solver_->mkNot(safeConstraint));

    if (solver_->check(constraints)) {
        // 找到反例 - 溢出
        auto model = solver_->getModel();
        reportVulnerability(inst, model);
    }
}
```

#### 1.4.2 NullPointerChecker

**文件位置**：`src/analyzer/Checkers/NullPointerChecker.h/cpp`

**职责**：检测空指针解引用

**类设计**：
```cpp
class NullPointerChecker : public IVulnerabilityChecker {
public:
    explicit NullPointerChecker(std::unique_ptr<ISMTSolver> solver);
    ~NullPointerChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "NullPointer"; }

private:
    void checkPointerDereference(SymbolicState* state, LLIRInstruction* inst);
    void checkPointerAccess(SymbolicState* state, Expr* ptr, LLIRInstruction* inst);

    std::unique_ptr<ISMTSolver> solver_;
};
```

#### 1.4.3 MemoryLeakChecker

**文件位置**：`src/analyzer/Checkers/MemoryLeakChecker.h/cpp`

**职责**：检测内存泄漏

**类设计**：
```cpp
class MemoryLeakChecker : public IVulnerabilityChecker {
public:
    MemoryLeakChecker();
    ~MemoryLeakChecker() override;

    std::vector<VulnerabilityReport> check(LLIRFunction* function) override;
    std::string getName() const override { return "MemoryLeak"; }

private:
    void trackAllocation(SymbolicState* state, Expr* ptr,
                         Expr* size, const SourceLocation& loc);
    void trackFree(SymbolicState* state, Expr* ptr);
    void checkLeaksAtExit(SymbolicState* state);

    std::map<AllocationSite*, std::vector<Expr*>> allocated_;
};
```

### 1.5 报告模块

#### 1.5.1 ReportGenerator

**文件位置**：`src/report/ReportGenerator.h/cpp`

**职责**：生成分析报告

**类设计**：
```cpp
class ReportGenerator {
public:
    static void generate(
        const AnalysisResult& result,
        const std::string& format,
        const std::string& outputFile = ""
    );
};
```

#### 1.5.2 ConsoleReporter

**文件位置**：`src/report/ConsoleReporter.h/cpp`

**职责**：生成控制台报告

**类设计**：
```cpp
class ConsoleReporter {
public:
    void generate(const AnalysisResult& result);

private:
    void printHeader();
    void printVulnerability(const VulnerabilityReport& report);
    void printSummary(const AnalysisResult& result);
};
```

**报告格式**：
```
[ERROR] Buffer Overflow Detected
  File: test.c, Line: 10, Column: 5
  Severity: CRITICAL
  Checker: buffer-overflow

  Code Snippet:
     9  char buf[10];
  >>10   buf[10] = 'a';
     11

  Error Trace:
    1. test.c:10 - Variable 'i' has value: 10
       Buffer 'buf' size: 10
       Access index: 10
       Condition: 10 < 10 is FALSE
```

#### 1.5.3 SarifExporter

**文件位置**：`src/report/SarifExporter.h/cpp`

**职责**：生成 SARIF 格式报告

**类设计**：
```cpp
class SarifExporter {
public:
    void exportToFile(
        const AnalysisResult& result,
        const std::string& filename
    );

private:
    nlohmann::json buildSarifLog(const AnalysisResult& result);
    nloh::json buildResult(const VulnerabilityReport& report);
};
```

---

## 2. 接口设计

### 2.1 模块间接口

#### 2.1.1 IParser 接口

**文件位置**：`include/cverifier/Frontend.h`

```cpp
class IParser {
public:
    virtual ~IParser() = default;

    virtual std::unique_ptr<LLIRModule> parse(
        const std::string& sourceFile,
        const ParseOptions& options = ParseOptions()
    ) = 0;

    virtual std::unique_ptr<LLIRModule> parseString(
        const std::string& sourceCode,
        const ParseOptions& options = ParseOptions()
    ) = 0;

    virtual std::string getLastError() const = 0;
};
```

#### 2.1.2 ISMTSolver 接口

**文件位置**：`include/cverifier/Analyzer.h`

```cpp
class ISMTSolver {
public:
    virtual ~ISMTSolver() = default;

    virtual bool check(const std::vector<Expr*>& constraints) = 0;
    virtual std::unordered_map<std::string, int64_t> getModel() = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
};
```

#### 2.1.3 IVulnerabilityChecker 接口

**文件位置**：`include/cverifier/Analyzer.h`

```cpp
class IVulnerabilityChecker {
public:
    virtual ~IVulnerabilityChecker() = default;

    virtual std::vector<VulnerabilityReport> check(
        LLIRFunction* function
    ) = 0;

    virtual std::string getName() const = 0;
};
```

### 2.2 外部接口

#### 2.2.1 命令行接口

**主函数**：`src/tools/cverifier.cpp`

```cpp
int main(int argc, char** argv) {
    // 1. 解析命令行
    auto options = parseCommandLine(argc, argv);

    // 2. 加载配置
    auto config = loadConfig(options.config);

    // 3. 创建分析器
    AnalysisManager analyzer(config);

    // 4. 运行分析
    auto result = analyzer.analyze(options.sources);

    // 5. 生成报告
    ReportGenerator::generate(result, options.format, options.output);

    return result.success ? 0 : 1;
}
```

#### 2.2.2 C++ API（可选）

**文件位置**：`include/cverifier/CVerifier.h`

```cpp
namespace cverifier {

class CVerifier {
public:
    CVerifier();
    ~CVerifier();

    void setConfig(const AnalysisConfig& config);
    AnalysisResult analyze(const std::vector<std::string>& sources);
    AnalysisResult analyzeString(const std::string& code);
};

}
```

---

## 3. 数据结构设计

### 3.1 LLIR 数据结构

#### 3.1.1 指令类型

```cpp
enum class LLIRInstructionType {
    // 算术运算
    Add, Sub, Mul, Div, Rem,
    And, Or, Xor, Shl, Shr,
    ICmp, FCmp,

    // 内存操作
    Alloca, Load, Store, GetElementPtr,

    // 控制流
    Br, Ret, Call,

    // 其他
    Phi, Select, Assert
};
```

#### 3.1.2 基本块

```cpp
class LLIRBasicBlock {
private:
    std::string name_;
    std::vector<LLIRInstruction*> instructions_;
    std::vector<LLIRBasicBlock*> successors_;
    std::vector<LLIRBasicBlock*> predecessors_;
};
```

### 3.2 符号状态数据结构

#### 3.2.1 表达式类型

```cpp
enum class ExprType {
    Constant, Variable, BinaryOp, UnaryOp, Boolean, NullPtr, Undefined
};
```

#### 3.2.2 符号状态

```cpp
class SymbolicState {
private:
    SymbolicStore store_;        // 变量 → 表达式
    SymbolicHeap heap_;          // 堆对象
    PathConstraint pathConstraint_; // 路径约束
    SymbolicState* parent_;      // 父状态
};
```

### 3.3 约束表达式数据结构

#### 3.3.1 二元操作

```cpp
enum class BinaryOpType {
    // 算术
    Add, Sub, Mul, Div, Rem,
    // 位运算
    And, Or, Xor, Shl, Shr,
    // 比较
    EQ, NE, LT, GT, LE, GE,
    // 逻辑
    LAnd, LOr
};
```

---

## 4. 算法设计

### 4.1 符号执行算法

#### 4.1.1 主循环

```
算法：SymbolicExecution

输入：LLIRFunction func
输出：AnalysisResult

1. 初始化：
   - worklist ← [(entryBlock, initialState)]
   - visitedStates ← ∅
   - vulnerabilities ← []

2. 当 worklist 不为空时：
   2.1 从 worklist 取出 (bb, state)
   2.2 如果 (bb, state) ∈ visitedStates，继续
   2.3 执行基本块 bb 中的每条指令
       for inst in bb.instructions:
           executeInstruction(state, inst)
       2.4 对于 bb 的每个后继 succ：
           newState ← state.clone()
           if succ 是条件分支：
               cond ← evaluateBranch(newState, succ)
               newState.addConstraint(cond)
           if solver.check(newState.constraints) == SAT:
               worklist.add((succ, newState))
   2.5 标记 (bb, state) 为已访问

3. 返回 vulnerabilities
```

#### 4.1.2 指令执行

```cpp
void executeInstruction(SymbolicState* state, LLIRInstruction* inst) {
    switch (inst->getType()) {
        case LLIRInstructionType::Add:
            executeAdd(state, inst);
            break;
        case LLIRInstructionType::Load:
            executeLoad(state, inst);
            break;
        case LLIRInstructionType::Store:
            executeStore(state, inst);
            break;
        case LLIRInstructionType::Br:
            executeBranch(state, inst);
            break;
        // ... 其他指令
    }
}
```

### 4.2 抽象解释算法

#### 4.2.1 不动点迭代

```
算法：FixpointIteration

输入：ControlFlowGraph cfg
输出：Map<BasicBlock, AbstractState>

1. 初始化：
   - states[entryBlock] ← initialState
   - states[otherBlocks] ⊥

2. repeat
   2.1 changed ← false
   2.2 for each bb in cfg.basicBlocks:
       oldState ← states[bb]
       newState ⊥ bottom
       2.2.1 for each pred in bb.predecessors:
           newState ← join(newState, states[pred])
       2.2.2 newState ← transfer(bb, newState)
       2.2.3 if iteration % wideningInterval = 0:
           newState ← widen(oldState, newState)
       2.2.4 if newState ≠ oldState:
           states[bb] ← newState
           changed ← true
3. until changed = false

4. 返回 states
```

### 4.3 路径探索算法

#### 4.3.1 DFS（深度优先）

```cpp
class DFSExplorer {
public:
    void explore(LLIRFunction* func) {
        auto* entry = func->getEntryBlock();
        exploreDFS(entry, initialState);
    }

private:
    void exploreDFS(LLIRBasicBlock* bb, SymbolicState* state) {
        // 执行基本块
        executeBasicBlock(state, bb);

        // 递归探索后继
        for (auto* succ : bb->getSuccessors()) {
            auto newState = state->clone();
            if (checkFeasibility(newState)) {
                exploreDFS(succ, newState);
            }
        }
    }
};
```

#### 4.3.2 BFS（广度优先）

```cpp
class BFSExplorer {
public:
    void explore(LLIRFunction* func) {
        std::queue<std::pair<LLIRBasicBlock*, SymbolicState*>> q;
        q.push({func->getEntryBlock(), initialState});

        while (!q.empty()) {
            auto [bb, state] = q.front();
            q.pop();

            executeBasicBlock(state, bb);

            for (auto* succ : bb->getSuccessors()) {
                auto newState = state->clone();
                if (checkFeasibility(newState)) {
                    q.push({succ, newState});
                }
            }
        }
    }
};
```

### 4.4 漏洞检测算法

#### 4.4.1 缓冲区溢出检测

```
算法：CheckBufferOverflow

输入：SymbolicState state, LoadInst inst
输出：VulnerabilityReport?

1. 提取信息：
   - baseAddr ← inst.getPointerOperand()
   - offset ← state.evaluate(inst.getOffset())
   - accessSize ← getTypeSize(inst.getType())

2. 获取缓冲区大小：
   - bufferSize ← getBufferSize(baseAddr)

3. 构造约束：
   - safe ← (offset ≥ 0) ∧ (offset + accessSize ≤ bufferSize)

4. 检查：
   - constraints ← state.pathConstraints ∪ {¬safe}
   - if solver.check(constraints) = SAT:
       model ← solver.getModel()
       report "Buffer Overflow" with model
```

#### 4.4.2 空指针解引用检测

```
算法：CheckNullPointer

输入：SymbolicState state, LoadInst inst
输出：VulnerabilityReport?

1. 提取信息：
   - ptr ← state.evaluate(inst.getPointerOperand())

2. 构造约束：
   - notNull ← (ptr ≠ NULL)

3. 检查：
   - constraints ← state.pathConstraints ∪ {¬notNull}
   - if solver.check(constraints) = SAT:
       model ← solver.getModel()
       report "Null Pointer Dereference" with model
```

---

## 5. 关键技术方案

### 5.1 状态空间爆炸解决方案

#### 5.1.1 抽象解释剪枝

**思路**：先用抽象解释快速判断路径可行性

```
对于每个状态：
  1. 使用抽象解释计算抽象状态
  2. 如果抽象状态 = ⊥（不可行），剪枝
  3. 如果抽象状态说明路径约束矛盾，剪枝
  4. 否则，使用符号执行精确分析
```

#### 5.1.2 状态合并

**思路**：合并相似状态

```cpp
SymbolicState* mergeStates(
    std::vector<SymbolicState*> states) {

    if (states.size() <= 1) return states[0];

    // 计算相似度
    for (size_t i = 0; i < states.size(); i++) {
        for (size_t j = i + 1; j < states.size(); j++) {
            if (areSimilar(states[i], states[j])) {
                // 合并状态
                return mergeTwo(states[i], states[j]);
            }
        }
    }
}
```

#### 5.1.3 路径深度限制

**思路**：限制最大探索深度

```cpp
if (state->getDepth() > maxDepth_) {
    // 应用 widening 或停止
    return;
}
```

### 5.2 上下文敏感分析方案

#### 5.2.1 函数摘要

**思路**：为函数生成摘要，避免重复分析

```cpp
struct FunctionSummary {
    std::vector<Expr*> inputConstraints;   // 前置条件
    std::vector<Expr*> outputConstraints;  // 后置条件
    std::map<Variable*, Expr*> sideEffects; // 副作用
};

class FunctionSummarizer {
public:
    FunctionSummary* summarize(LLIRFunction* func);
    void applySummary(SymbolicState* state, FunctionSummary* summary,
                      std::vector<Expr*> args);
};
```

#### 5.2.2 k-CFA 策略

**思路**：限制调用栈深度

```cpp
std::string getContextKey(SymbolicState* state) {
    auto callStack = state->getCallStack();

    // 只保留最近 k 个调用点
    const int K = 2;
    if (callStack.size() > K) {
        callStack.resize(K);
    }

    return hashCallStack(callStack);
}
```

### 5.3 外部函数建模方案

#### 5.3.1 标准库模型

```cpp
class StdLibModels {
public:
    void modelMalloc(SymbolicState* state, CallInst* call) {
        Expr* size = state->getArg(call, 0);
        Expr* addr = state->heap->allocate(size, call->getLocation());

        // 可能返回 NULL
        Expr* ret = solver->mkITE(
            solver->mkLe(size, solver->mkInt(0)),
            solver->mkNull(),
            addr
        );

        state->setReturn(call, ret);
    }

    void modelMemcpy(SymbolicState* state, CallInst* call) {
        Expr* dest = state->getArg(call, 0);
        Expr* src = state->getArg(call, 1);
        Expr* n = state->getArg(call, 2);

        // 添加约束: src 和 dest 不应重叠
        // 更新堆: dest[i] = src[i] for i in [0, n)

        state->heap->memcpy(dest, src, n);
        state->setReturn(call, dest);
    }
};
```

### 5.4 性能优化方案

#### 5.4.1 并行路径探索

```cpp
class ParallelPathExplorer {
    ThreadPool pool;
    std::mutex worklistMutex;
    std::atomic<bool> stopFlag;

public:
    void explore(WorkList& initialWorklist) {
        while (!initialWorklist.empty() && !stopFlag) {
            auto [state, bb] = initialWorklist.pop();

            pool.enqueue([this, state, bb]() {
                executeBasicBlock(state, bb);
            });
        }
    }
};
```

#### 5.4.2 增量分析

```cpp
class IncrementalAnalyzer {
    AnalysisCache cache;

public:
    void analyze(File* file) {
        auto hash = computeHash(file);

        if (cache.hasValidResult(hash)) {
            applyCachedResult(cache.getResult(hash));
            return;
        }

        auto result = performAnalysis(file);
        cache.storeResult(hash, result);
    }
};
```

---

## 6. 规约系统设计 ⭐新增

### 6.1 C语言验证API设计

**文件位置**：`specs/verification.h`

**接口定义**：
```c
#ifndef VERIFICATION_H
#define VERIFICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// 内存验证函数
// ============================================================================

/**
 * 验证内存可读性
 * @param ptr 内存指针
 * @param size 字节数
 * @return true 如果内存可读，false 否则
 */
bool verifiable_read(void* ptr, int size);

/**
 * 验证内存可写性
 * @param ptr 内存指针
 * @param size 字节数
 * @return true 如果内存可写，false 否则
 */
bool verifiable_write(void* ptr, int size);

/**
 * 验证指针有效性（非NULL且在合法范围内）
 * @param ptr 指针
 * @return true 如果指针有效，false 否则
 */
bool verifiable_pointer(void* ptr);

/**
 * 验证两块内存不重叠
 * @param ptr1 第一块内存
 * @param ptr2 第二块内存
 * @param size 内存大小
 * @return true 如果内存分离，false 否则
 */
bool verifiable_separated(void* ptr1, void* ptr2, int size);

// ============================================================================
// 数值验证函数
// ============================================================================

/**
 * 验证值在指定范围内
 * @param value 待验证的值
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return true 如果在范围内，false 否则
 */
bool in_range(int value, int min, int max);

/**
 * 验证无符号整数为正数
 * @param value 待验证的值
 * @return true 如果为正数，false 否则
 */
bool is_positive(int value);

/**
 * 验证整数为非零
 * @param value 待验证的值
 * @return true 如果非零，false 否则
 */
bool is_nonzero(int value);

/**
 * 验证无符号整型溢出
 * @param a 操作数1
 * @param b 操作数2
 * @return true 如果 a + b 会溢出，false 否则
 */
bool will_add_overflow(unsigned int a, unsigned int b);

/**
 * 验证有符号整型溢出
 * @param a 操作数1
 * @param b 操作数2
 * @return true 如果 a + b 会溢出，false 否则
 */
bool will_signed_add_overflow(int a, int b);

// ============================================================================
// 浮点验证函数 (IEEE 754)
// ============================================================================

/**
 * 检测浮点数是否为 NaN
 * @param value 浮点数值
 * @return true 如果是 NaN，false 否则
 */
bool is_nan(double value);

/**
 * 检测浮点数是否为无穷大
 * @param value 浮点数值
 * @return true 如果是无穷大，false 否则
 */
bool is_infinity(double value);

/**
 * 检测浮点数是否为有限数（非 NaN，非 Inf）
 * @param value 浮点数值
 * @return true 如果是有限数，false 否则
 */
bool is_finite(double value);

/**
 * 检测浮点乘法是否会上溢
 * @param a 操作数1
 * @param b 操作数2
 * @return true 如果 a * b 会溢出到 Inf，false 否则
 */
bool will_multiply_overflow(double a, double b);

/**
 * 检测浮点运算精度损失
 * @param large 大数
 * @param small 小数
 * @return true 如果 large + small 会导致精度损失，false 否则
 */
bool will_lose_precision(double large, double small);

// ============================================================================
// 断言函数
// ============================================================================

/**
 * 运行时断言
 * @param condition 条件
 * @param msg 错误消息
 */
void assert_true(bool condition, const char* msg);

/**
 * 静态断言（编译时）
 * @param condition 编译时常量
 */
#define STATIC_ASSERT(condition) _Static_assert(condition, #condition)

/**
 * 运行时检查（规约验证专用）
 * @param condition 条件
 * @param file 源文件
 * @param line 行号
 * @return true 如果条件成立，false 否则（不终止程序）
 */
bool verification_check(bool condition, const char* file, int line);

#define VERIFICATION_CHECK(cond) \
    verification_check(cond, __FILE__, __LINE__)

// ============================================================================
// ACSL 风格谓词（C语言实现）
// ============================================================================

/**
 * ACSL \valid 谓词实现
 * 检查指针指向的内存区域可读
 */
#define valid(ptr) verifiable_read(ptr, sizeof(*(ptr)))

/**
 * ACSL \valid_read 谓词实现
 */
#define valid_read(ptr) verifiable_read(ptr, sizeof(*(ptr)))

/**
 * ACSL \separated 谓词实现
 * 检查两个指针指向的内存不重叠
 */
#define separated(ptr1, ptr2) \
    verifiable_separated(ptr1, ptr2, sizeof(*(ptr1)))

/**
 * ACSL \null 谓词实现
 */
#define null(ptr) ((ptr) == NULL)

/**
 * ACSL \initialized 谓词实现
 * 检查指针是否已初始化（非 NULL）
 */
#define initialized(ptr) ((ptr) != NULL)

#ifdef __cplusplus
}
#endif

#endif // VERIFICATION_H
```

**由于篇幅限制，完整的插件系统、浮点检测算法和Web API设计将创建为独立文档。**

详见：
- [插件 API 文档](plugin-api.md) - 插件接口、加载器、沙箱设计
- [验证 API 文档](verification-api.md) - C语言验证API完整参考
- [Web 架构文档](web-architecture.md) - REST/WebSocket API、前端设计

---

**文档结束**
