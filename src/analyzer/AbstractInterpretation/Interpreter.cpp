/**
 * @file AbstractInterpreter.cpp
 * @brief 抽象解释器实现
 */

#include "cverifier/AbstractInterpreter.h"
#include "cverifier/LLIRFactory.h"
#include "cverifier/Utils.h"
#include <sstream>
#include <algorithm>
#include <queue>

namespace cverifier {
namespace core {

// ============================================================================
// AbstractValue 实现
// ============================================================================

AbstractValue* AbstractValue::createTop(ValueType type) {
    return IntervalValue::createTop(type);
}

AbstractValue* AbstractValue::createBottom(ValueType type) {
    return IntervalValue::createBottom(type);
}

// ============================================================================
// AbstractStore 实现
// ============================================================================

AbstractStore::~AbstractStore() {
    for (auto& [var, value] : bindings_) {
        delete value;
    }
}

void AbstractStore::bind(const std::string& var, AbstractValue* value) {
    // 删除旧值
    auto it = bindings_.find(var);
    if (it != bindings_.end()) {
        delete it->second;
    }

    // 插入新值
    bindings_[var] = value;
}

AbstractValue* AbstractStore::lookup(const std::string& var) const {
    auto it = bindings_.find(var);
    if (it != bindings_.end()) {
        return it->second;
    }
    return nullptr;
}

AbstractStore* AbstractStore::merge(const AbstractStore* other) const {
    auto* result = new AbstractStore();

    // 合并两个存储的绑定
    std::unordered_set<std::string> allVars;

    for (const auto& [var, _] : bindings_) {
        allVars.insert(var);
    }

    for (const auto& [var, _] : other->bindings_) {
        allVars.insert(var);
    }

    // 对每个变量执行join操作
    for (const auto& var : allVars) {
        AbstractValue* value1 = lookup(var);
        AbstractValue* value2 = other->lookup(var);

        if (!value1) {
            if (value2) {
                result->bind(var, value2->clone());
            }
        } else if (!value2) {
            result->bind(var, value1->clone());
        } else {
            // 两个值都存在，执行join
            // 这里简化处理：如果相等则使用该值，否则使用Top
            if (value1->equals(value2)) {
                result->bind(var, value1->clone());
            } else {
                result->bind(var, AbstractValue::createTop(value1->getType()));
            }
        }
    }

    return result;
}

AbstractStore* AbstractStore::clone() const {
    auto* result = new AbstractStore();

    for (const auto& [var, value] : bindings_) {
        result->bind(var, value->clone());
    }

    return result;
}

std::string AbstractStore::toString() const {
    std::ostringstream oss;
    oss << "{";

    bool first = true;
    for (const auto& [var, value] : bindings_) {
        if (!first) {
            oss << ", ";
        }
        first = false;

        oss << var << " = " << value->toString();
    }

    oss << "}";
    return oss.str();
}

bool AbstractStore::lessOrEqual(const AbstractStore* other) const {
    // 简化实现：检查当前存储的所有变量是否在other中也有值
    // 且值相等或更精确

    for (const auto& [var, value] : bindings_) {
        AbstractValue* otherValue = other->lookup(var);

        if (!otherValue) {
            // other没有这个变量，可能less-or-equal不成立
            return false;
        }

        // 如果other的值是Top，则current less-or-equal other
        if (otherValue->isTop()) {
            continue;
        }

        // 如果值不相等，则不满足less-or-equal
        if (!value->equals(otherValue)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// IntervalTransferFunction 实现
// ============================================================================

namespace {

/**
 * @brief 区间算术运算辅助函数
 */
IntervalValue* intervalAdd(IntervalValue* a, IntervalValue* b) {
    if (a->isTop() || b->isTop()) {
        return IntervalValue::createTop(ValueType::Integer);
    }

    if (a->isBottom() || b->isBottom()) {
        return IntervalValue::createBottom(ValueType::Integer);
    }

    if (!a->isBounded() || !b->isBounded()) {
        return IntervalValue::createTop(ValueType::Integer);
    }

    // [a.low, a.high] + [b.low, b.high] = [a.low + b.low, a.high + b.high]
    int64_t newLow = a->getLowInt() + b->getLowInt();
    int64_t newHigh = a->getHighInt() + b->getHighInt();

    return new IntervalValue(newLow, newHigh);
}

IntervalValue* intervalSub(IntervalValue* a, IntervalValue* b) {
    if (a->isTop() || b->isTop()) {
        return IntervalValue::createTop(ValueType::Integer);
    }

    if (a->isBottom() || b->isBottom()) {
        return IntervalValue::createBottom(ValueType::Integer);
    }

    if (!a->isBounded() || !b->isBounded()) {
        return IntervalValue::createTop(ValueType::Integer);
    }

    // [a.low, a.high] - [b.low, b.high] = [a.low - b.high, a.high - b.low]
    int64_t newLow = a->getLowInt() - b->getHighInt();
    int64_t newHigh = a->getHighInt() - b->getLowInt();

    return new IntervalValue(newLow, newHigh);
}

IntervalValue* intervalMul(IntervalValue* a, IntervalValue* b) {
    if (a->isTop() || b->isTop()) {
        return IntervalValue::createTop(ValueType::Integer);
    }

    if (a->isBottom() || b->isBottom()) {
        return IntervalValue::createBottom(ValueType::Integer);
    }

    if (!a->isBounded() || !b->isBounded()) {
        return IntervalValue::createTop(ValueType::Integer);
    }

    // [a.low, a.high] * [b.low, b.high]
    // 需要计算4种组合，取最小和最大
    int64_t products[4] = {
        a->getLowInt() * b->getLowInt(),
        a->getLowInt() * b->getHighInt(),
        a->getHighInt() * b->getLowInt(),
        a->getHighInt() * b->getHighInt()
    };

    int64_t newLow = *std::min_element(products, products + 4);
    int64_t newHigh = *std::max_element(products, products + 4);

    return new IntervalValue(newLow, newHigh);
}

IntervalValue* intervalDiv(IntervalValue* a, IntervalValue* b) {
    // 除法比较复杂，简化处理
    return IntervalValue::createTop(ValueType::Integer);
}

IntervalValue* intervalCmp(IntervalValue* a, IntervalValue* b) {
    // 比较操作，简化处理
    return IntervalValue::createTop(ValueType::Integer);
}

} // anonymous namespace

AbstractStore* IntervalTransferFunction::transfer(
    LLIRInstruction* inst,
    const AbstractStore* store
) const {
    // 克隆当前存储
    AbstractStore* newStore = store->clone();

    auto type = inst->getType();

    switch (type) {
        case LLIRInstructionType::Add:
        case LLIRInstructionType::Sub:
        case LLIRInstructionType::Mul:
        case LLIRInstructionType::Div: {
            // 算术运算：创建临时变量
            std::string resultVar = freshVarName("op");

            // 获取操作数（简化处理：假设是常量）
            const auto& operands = inst->getOperands();
            if (operands.size() >= 2) {
                // 尝试获取操作数的区间值
                // 这里简化处理，实际需要从符号值转换
                auto* result = IntervalValue::createTop(ValueType::Integer);
                newStore->bind(resultVar, result);
            }

            break;
        }

        case LLIRInstructionType::Alloca: {
            // 栈分配：创建新变量
            std::string varName = freshVarName("alloca");
            auto* value = IntervalValue::createTop(ValueType::Integer);
            newStore->bind(varName, value);
            break;
        }

        case LLIRInstructionType::Load: {
            // 加载：读取内存
            // 简化处理：返回Top
            std::string varName = freshVarName("load");
            auto* value = IntervalValue::createTop(ValueType::Integer);
            newStore->bind(varName, value);
            break;
        }

        case LLIRInstructionType::Store: {
            // 存储：写入内存
            // 简化处理：不更新存储
            break;
        }

        case LLIRInstructionType::Ret: {
            // 返回：不需要特殊处理
            break;
        }

        case LLIRInstructionType::Br: {
            // 分支：不需要特殊处理
            break;
        }

        case LLIRInstructionType::Call: {
            // 函数调用：简化处理
            // 可以在这里进行函数摘要分析
            break;
        }

        default:
            // 其他指令：暂不处理
            break;
    }

    return newStore;
}

std::string freshVarName(const std::string& prefix) {
    static int counter = 0;
    return prefix + "_" + std::to_string(counter++);
}

// ============================================================================
// FixpointIterator 实现
// ============================================================================

FixpointIterator::FixpointIterator(CFG* cfg, TransferFunction* transferFunc)
    : cfg_(cfg), transferFunc_(transferFunc), iterations_(0) {
}

FixpointIterator::~FixpointIterator() {
    // 清理资源
}

std::unordered_map<std::string, AbstractStore*> FixpointIterator::compute() {
    utils::Logger::info("Computing fixpoint for CFG");

    // 初始化：所有基本块的输入为Top
    std::unordered_map<std::string, AbstractStore*> inStates;
    std::unordered_map<std::string, AbstractStore*> outStates;

    for (const auto& [name, node] : cfg_->getNodes()) {
        inStates[name] = new AbstractStore();
        outStates[name] = new AbstractStore();
    }

    // 工作列表算法
    std::queue<CFGNode*> worklist;

    // 将入口节点加入工作列表
    CFGNode* entryNode = cfg_->getEntryNode();
    if (entryNode) {
        worklist.push(entryNode);
    }

    // 初始化入口节点的输入为空存储
    inStates[entryNode->getId()] = new AbstractStore();

    while (!worklist.empty()) {
        // 弹出一个节点
        CFGNode* node = worklist.front();
        worklist.pop();

        std::string nodeId = node->getId();
        utils::Logger::debug("Processing basic block: " + nodeId);

        // 获取输入状态
        AbstractStore* inState = inStates[nodeId];

        // 执行转移函数
        AbstractStore* outState = inState;
        LLIRBasicBlock* bb = node->getBasicBlock();

        for (auto* inst : bb->getInstructions()) {
            AbstractStore* newState = transferFunc_->transfer(inst, outState);
            delete outState;
            outState = newState;
        }

        // 检查是否发生变化
        AbstractStore* oldOutState = outStates[nodeId];

        if (!outState->lessOrEqual(oldOutState)) {
            // 状态发生变化，更新并添加后继到工作列表
            delete oldOutState;
            outStates[nodeId] = outState;

            for (auto* succ : node->getSuccessors()) {
                worklist.push(succ);
            }
        } else {
            // 状态未变化，丢弃新状态
            delete outState;
        }

        iterations_++;

        // 防止无限循环
        if (iterations_ > 10000) {
            utils::Logger::warning("Fixpoint iteration exceeded maximum limit");
            break;
        }
    }

    utils::Logger::info("Fixpoint computation completed in " + std::to_string(iterations_) + " iterations");

    // 返回输出状态（或合并的输入+输出状态）
    return outStates;
}

// ============================================================================
// AbstractInterpreter 实现
// ============================================================================

AbstractInterpreter::AbstractInterpreter(LLIRModule* module)
    : module_(module), domain_("interval"), functionsAnalyzed_(0), analysisTime_(0.0) {
}

AbstractInterpreter::~AbstractInterpreter() {
    // 清理结果
    for (auto& [name, store] : results_) {
        delete store;
    }
}

void AbstractInterpreter::analyzeFunction(const std::string& functionName) {
    utils::Logger::info("Analyzing function with abstract interpretation: " + functionName);

    utils::Timer timer;

    auto* func = module_->getFunction(functionName);
    if (!func) {
        utils::Logger::error("Function not found: " + functionName);
        return;
    }

    // 创建CFG
    CFG cfg(func);

    // 创建转移函数
    TransferFunction* transferFunc = nullptr;

    if (domain_ == "interval") {
        transferFunc = new IntervalTransferFunction();
    } else {
        utils::Logger::warning("Unknown domain: " + domain_ + ", using interval");
        transferFunc = new IntervalTransferFunction();
    }

    // 计算不动点
    FixpointIterator fixpoint(&cfg, transferFunc);
    results_ = fixpoint.compute();

    delete transferFunc;

    functionsAnalyzed_++;
    analysisTime_ = timer.elapsedSec();

    utils::Logger::info("Abstract interpretation completed in " + std::to_string(analysisTime_) + "s");
}

void AbstractInterpreter::analyzeModule() {
    utils::Logger::info("Analyzing module with abstract interpretation");

    for (auto* func : module_->getFunctions()) {
        analyzeFunction(func->getName());
    }
}

std::string AbstractInterpreter::getStatistics() const {
    std::ostringstream oss;

    oss << "Abstract Interpretation Statistics:\n";
    oss << "  Domain: " << domain_ << "\n";
    oss << "  Functions Analyzed: " << functionsAnalyzed_ << "\n";
    oss << "  Analysis Time: " << std::fixed << analysisTime_ << "s\n";
    oss << "  Basic Blocks Analyzed: " << results_.size() << "\n";

    return oss.str();
}

} // namespace core
} // namespace cverifier
