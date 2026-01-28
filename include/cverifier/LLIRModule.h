#ifndef CVERIFIER_LLIR_MODULE_H
#define CVERIFIER_LLIR_MODULE_H

#include "cverifier/Core.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace cverifier {
namespace core {

// ============================================================================
// LLIR - 轻量级LLVM IR表示
// ============================================================================

/**
 * @brief LLIR指令类型枚举
 */
enum class LLIRInstructionType {
    // 算术运算
    Add,        ///< 加法
    Sub,        ///< 减法
    Mul,        ///< 乘法
    Div,        ///< 除法
    Rem,        ///< 取余

    // 位运算
    And,        ///< 按位与
    Or,         ///< 按位或
    Xor,        ///< 按位异或
    Shl,        ///< 左移
    Shr,        ///< 右移

    // 比较运算
    ICmp,       ///< 整数比较
    FCmp,       ///< 浮点比较

    // 内存操作
    Alloca,     ///< 栈上分配
    Load,       ///< 加载
    Store,      ///< 存储
    GetElementPtr,  ///< 获取元素指针

    // 控制流
    Br,         ///< 分支
    Ret,        ///< 返回
    Call,       ///< 函数调用

    // 其他
    Phi,        ///< PHI节点
    Select,     ///< 选择
    Assert      ///< 断言（用于漏洞检测）
};

/**
 * @brief 源代码位置信息
 */
struct SourceLocation {
    std::string file;
    int line = 0;
    int column = 0;

    bool isValid() const { return !file.empty(); }
    std::string toString() const;
};

/**
 * @brief LLIR值基类
 */
class LLIRValue {
public:
    virtual ~LLIRValue() = default;

    virtual std::string toString() const = 0;
    virtual ValueType getValueType() const = 0;
};

/**
 * @brief LLIR指令类
 */
class LLIRInstruction : public LLIRValue {
public:
    LLIRInstruction(LLIRInstructionType type, SourceLocation loc = {})
        : type_(type), location_(loc) {}

    virtual ~LLIRInstruction() = default;

    LLIRInstructionType getType() const { return type_; }
    SourceLocation getLocation() const { return location_; }

    void addOperand(LLIRValue* operand) {
        operands_.push_back(operand);
    }

    const std::vector<LLIRValue*>& getOperands() const {
        return operands_;
    }

    std::string toString() const override;

private:
    LLIRInstructionType type_;
    SourceLocation location_;
    std::vector<LLIRValue*> operands_;
};

/**
 * @brief LLIR基本块
 */
class LLIRBasicBlock {
public:
    LLIRBasicBlock(const std::string& name = "")
        : name_(name) {}

    void addInstruction(LLIRInstruction* inst) {
        instructions_.push_back(inst);
    }

    void addSuccessor(LLIRBasicBlock* succ) {
        successors_.push_back(succ);
    }

    void addPredecessor(LLIRBasicBlock* pred) {
        predecessors_.push_back(pred);
    }

    const std::vector<LLIRInstruction*>& getInstructions() const {
        return instructions_;
    }

    const std::vector<LLIRBasicBlock*>& getSuccessors() const {
        return successors_;
    }

    const std::vector<LLIRBasicBlock*>& getPredecessors() const {
        return predecessors_;
    }

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    size_t size() const { return instructions_.size(); }
    bool empty() const { return instructions_.empty(); }

private:
    std::string name_;
    std::vector<LLIRInstruction*> instructions_;
    std::vector<LLIRBasicBlock*> successors_;
    std::vector<LLIRBasicBlock*> predecessors_;
};

/**
 * @brief LLIR函数
 */
class LLIRFunction {
public:
    LLIRFunction(const std::string& name)
        : name_(name) {}

    void addBasicBlock(LLIRBasicBlock* bb) {
        basicBlocks_.push_back(bb);
        bbMap_[bb->getName()] = bb;
    }

    void setEntryBlock(LLIRBasicBlock* entry) {
        entryBlock_ = entry;
    }

    LLIRBasicBlock* getEntryBlock() const { return entryBlock_; }

    LLIRBasicBlock* getBasicBlock(const std::string& name) const {
        auto it = bbMap_.find(name);
        return it != bbMap_.end() ? it->second : nullptr;
    }

    const std::vector<LLIRBasicBlock*>& getBasicBlocks() const {
        return basicBlocks_;
    }

    std::string getName() const { return name_; }

private:
    std::string name_;
    std::vector<LLIRBasicBlock*> basicBlocks_;
    std::unordered_map<std::string, LLIRBasicBlock*> bbMap_;
    LLIRBasicBlock* entryBlock_ = nullptr;
};

/**
 * @brief LLIR模块
 */
class LLIRModule {
public:
    LLIRModule(const std::string& name = "module")
        : name_(name) {}

    ~LLIRModule();

    void addFunction(LLIRFunction* func) {
        functions_.push_back(func);
        funcMap_[func->getName()] = func;
    }

    LLIRFunction* getFunction(const std::string& name) const {
        auto it = funcMap_.find(name);
        return it != funcMap_.end() ? it->second : nullptr;
    }

    const std::vector<LLIRFunction*>& getFunctions() const {
        return functions_;
    }

    std::string getName() const { return name_; }

    /// 验证LLIR的合法性
    bool validate() const;

    /// 打印LLIR（用于调试）
    std::string dump() const;

private:
    std::string name_;
    std::vector<LLIRFunction*> functions_;
    std::unordered_map<std::string, LLIRFunction*> funcMap_;
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_LLIR_MODULE_H
