/**
 * @file LLIRModule.cpp
 * @brief LLIR 模块实现
 */

#include "cverifier/LLIRModule.h"
#include "cverifier/Utils.h"
#include <sstream>

namespace cverifier {
namespace core {

// ============================================================================
// LLIRInstruction 实现
// ============================================================================

std::string LLIRInstruction::toString() const {
    std::ostringstream oss;

    // 指令名称
    switch (type_) {
        case LLIRInstructionType::Add:
            oss << "add";
            break;
        case LLIRInstructionType::Sub:
            oss << "sub";
            break;
        case LLIRInstructionType::Mul:
            oss << "mul";
            break;
        case LLIRInstructionType::Div:
            oss << "div";
            break;
        case LLIRInstructionType::Rem:
            oss << "rem";
            break;
        case LLIRInstructionType::And:
            oss << "and";
            break;
        case LLIRInstructionType::Or:
            oss << "or";
            break;
        case LLIRInstructionType::Xor:
            oss << "xor";
            break;
        case LLIRInstructionType::Shl:
            oss << "shl";
            break;
        case LLIRInstructionType::Shr:
            oss << "shr";
            break;
        case LLIRInstructionType::ICmp:
            oss << "icmp";
            break;
        case LLIRInstructionType::FCmp:
            oss << "fcmp";
            break;
        case LLIRInstructionType::Alloca:
            oss << "alloca";
            break;
        case LLIRInstructionType::Load:
            oss << "load";
            break;
        case LLIRInstructionType::Store:
            oss << "store";
            break;
        case LLIRInstructionType::GetElementPtr:
            oss << "getelementptr";
            break;
        case LLIRInstructionType::Br:
            oss << "br";
            break;
        case LLIRInstructionType::Ret:
            oss << "ret";
            break;
        case LLIRInstructionType::Call:
            oss << "call";
            break;
        case LLIRInstructionType::Phi:
            oss << "phi";
            break;
        case LLIRInstructionType::Select:
            oss << "select";
            break;
        case LLIRInstructionType::Assert:
            oss << "assert";
            break;
        default:
            oss << "unknown";
            break;
    }

    // 操作数
    for (size_t i = 0; i < operands_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << operands_[i]->toString();
    }

    return oss.str();
}

// ============================================================================
// LLIRModule 实现
// ============================================================================

LLIRModule::~LLIRModule() {
    // 清理所有函数
    for (auto* func : functions_) {
        delete func;
    }

    // 清理所有基本块和指令
    // 注意：由于基本块和指令由函数管理，这里不需要额外清理
}

bool LLIRModule::validate() const {
    // 基本验证
    if (functions_.empty()) {
        return false;  // 模块至少要有一个函数
    }

    // 验证每个函数
    for (const auto* func : functions_) {
        if (func == nullptr) {
            return false;
        }

        // 每个函数至少要有一个基本块
        if (func->getBasicBlocks().empty()) {
            return false;
        }

        // 验证入口块
        if (func->getEntryBlock() == nullptr) {
            return false;
        }

        // 验证基本块的连通性（简化验证）
        for (const auto* bb : func->getBasicBlocks()) {
            if (bb == nullptr) {
                return false;
            }

            // 基本块名称不能为空
            if (bb->getName().empty()) {
                return false;
            }
        }
    }

    return true;
}

std::string LLIRModule::dump() const {
    std::ostringstream oss;

    oss << "LLIR Module: " << name_ << "\n";
    oss << "Functions: " << functions_.size() << "\n";

    for (const auto* func : functions_) {
        oss << "\n  Function: " << func->getName() << "\n";

        for (const auto* bb : func->getBasicBlocks()) {
            oss << "    BasicBlock: " << bb->getName() << "\n";
            oss << "      Instructions: " << bb->size() << "\n";

            for (const auto* inst : bb->getInstructions()) {
                oss << "        " << inst->toString();
                if (inst->getLocation().isValid()) {
                    oss << "  ; " << inst->getLocation().toString();
                }
                oss << "\n";
            }

            // 后继
            if (!bb->getSuccessors().empty()) {
                oss << "      Successors: ";
                for (size_t i = 0; i < bb->getSuccessors().size(); ++i) {
                    if (i > 0) {
                        oss << ", ";
                    }
                    oss << bb->getSuccessors()[i]->getName();
                }
                oss << "\n";
            }
        }
    }

    return oss.str();
}

} // namespace core
} // namespace cverifier
