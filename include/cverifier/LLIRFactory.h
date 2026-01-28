#ifndef CVERIFIER_LLIR_FACTORY_H
#define CVERIFIER_LLIR_FACTORY_H

#include "cverifier/LLIRModule.h"
#include "cverifier/LLIRValue.h"
#include "cverifier/Core.h"
#include <string>

namespace cverifier {
namespace core {

/**
 * @brief LLIR指令工厂类
 *
 * 提供便捷的方法来创建各种类型的LLIR指令
 */
class LLIRFactory {
public:
    // ========================================================================
    // 算术运算指令
    // ========================================================================

    static LLIRInstruction* createAdd(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Add, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createSub(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Sub, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createMul(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Mul, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createDiv(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Div, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createRem(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Rem, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    // ========================================================================
    // 位运算指令
    // ========================================================================

    static LLIRInstruction* createAnd(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::And, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createOr(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Or, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createXor(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Xor, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createShl(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Shl, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createShr(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Shr, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    // ========================================================================
    // 比较运算指令
    // ========================================================================

    static LLIRInstruction* createICmp(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::ICmp, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    static LLIRInstruction* createFCmp(
        LLIRValue* left,
        LLIRValue* right,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::FCmp, loc);
        inst->addOperand(left);
        inst->addOperand(right);
        return inst;
    }

    // ========================================================================
    // 内存操作指令
    // ========================================================================

    static LLIRInstruction* createAlloca(
        LLIRValue* size,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Alloca, loc);
        inst->addOperand(size);
        return inst;
    }

    static LLIRInstruction* createLoad(
        LLIRValue* ptr,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Load, loc);
        inst->addOperand(ptr);
        return inst;
    }

    static LLIRInstruction* createStore(
        LLIRValue* value,
        LLIRValue* ptr,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Store, loc);
        inst->addOperand(value);
        inst->addOperand(ptr);
        return inst;
    }

    static LLIRInstruction* createGetElementPtr(
        LLIRValue* ptr,
        LLIRValue* index,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::GetElementPtr, loc);
        inst->addOperand(ptr);
        inst->addOperand(index);
        return inst;
    }

    // ========================================================================
    // 控制流指令
    // ========================================================================

    static LLIRInstruction* createBr(
        LLIRBasicBlock* target,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Br, loc);
        // 使用基本块名称作为操作数
        auto* nameVar = new LLIRVariable(target->getName(), ValueType::Void);
        inst->addOperand(nameVar);
        return inst;
    }

    static LLIRInstruction* createConditionalBr(
        LLIRValue* condition,
        LLIRBasicBlock* thenBlock,
        LLIRBasicBlock* elseBlock,
        SourceLocation loc = {}
    ) {
        // 条件分支需要特殊处理，这里简化为无条件分支
        // 实际实现中应该创建一个包含条件和两个目标的基本块
        auto* inst = new LLIRInstruction(LLIRInstructionType::Br, loc);
        inst->addOperand(condition);
        return inst;
    }

    static LLIRInstruction* createRet(
        LLIRValue* value = nullptr,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Ret, loc);
        if (value) {
            inst->addOperand(value);
        }
        return inst;
    }

    static LLIRInstruction* createCall(
        const std::string& functionName,
        const std::vector<LLIRValue*>& args,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Call, loc);
        // 添加函数名
        auto* funcVar = new LLIRVariable(functionName, ValueType::Void);
        inst->addOperand(funcVar);
        // 添加参数
        for (auto* arg : args) {
            inst->addOperand(arg);
        }
        return inst;
    }

    // ========================================================================
    // 其他指令
    // ========================================================================

    static LLIRInstruction* createPhi(
        const std::vector<LLIRValue*>& incomingValues,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Phi, loc);
        for (auto* value : incomingValues) {
            inst->addOperand(value);
        }
        return inst;
    }

    static LLIRInstruction* createSelect(
        LLIRValue* condition,
        LLIRValue* trueValue,
        LLIRValue* falseValue,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Select, loc);
        inst->addOperand(condition);
        inst->addOperand(trueValue);
        inst->addOperand(falseValue);
        return inst;
    }

    static LLIRInstruction* createAssert(
        LLIRValue* condition,
        const std::string& message,
        SourceLocation loc = {}
    ) {
        auto* inst = new LLIRInstruction(LLIRInstructionType::Assert, loc);
        inst->addOperand(condition);
        // 将消息附加到指令中（通过toString展示）
        return inst;
    }

    // ========================================================================
    // 基本块和函数创建
    // ========================================================================

    static LLIRBasicBlock* createBasicBlock(const std::string& name = "") {
        return new LLIRBasicBlock(name);
    }

    static LLIRFunction* createFunction(const std::string& name) {
        return new LLIRFunction(name);
    }

    static LLIRModule* createModule(const std::string& name = "module") {
        return new LLIRModule(name);
    }

    // ========================================================================
    // 值创建
    // ========================================================================

    static LLIRConstant* createIntConstant(int64_t value) {
        return new LLIRConstant(value);
    }

    static LLIRConstant* createFloatConstant(double value) {
        return new LLIRConstant(value);
    }

    static LLIRConstant* createNullConstant() {
        return LLIRConstant::createNull();
    }

    static LLIRVariable* createVariable(
        const std::string& name,
        ValueType type,
        int id = -1
    ) {
        return new LLIRVariable(name, type, id);
    }

    static LLIRArgument* createArgument(
        const std::string& name,
        ValueType type,
        int index
    ) {
        return new LLIRArgument(name, type, index);
    }

    static LLIRGlobalVariable* createGlobalVariable(
        const std::string& name,
        ValueType type,
        bool isConst = false
    ) {
        return new LLIRGlobalVariable(name, type, isConst);
    }
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_LLIR_FACTORY_H
