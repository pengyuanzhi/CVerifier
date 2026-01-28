#ifndef CVERIFIER_LLIR_VALUE_H
#define CVERIFIER_LLIR_VALUE_H

#include "cverifier/Core.h"
#include "cverifier/LLIRModule.h"
#include <string>

namespace cverifier {
namespace core {

// ============================================================================
// LLIR 具体值类型
// ============================================================================

/**
 * @brief LLIR常量
 */
class LLIRConstant : public LLIRValue {
public:
    enum class ConstantType {
        Integer,
        Float,
        Null,
        Undef
    };

    LLIRConstant(int64_t value)
        : type_(ConstantType::Integer), intValue_(value) {}

    LLIRConstant(double value)
        : type_(ConstantType::Float), floatValue_(value) {}

    LLIRConstant(ConstantType type)
        : type_(type) {
        if (type == ConstantType::Integer) {
            intValue_ = 0;
        } else if (type == ConstantType::Float) {
            floatValue_ = 0.0;
        }
    }

    static LLIRConstant* createNull() {
        return new LLIRConstant(ConstantType::Null);
    }

    static LLIRConstant* createUndef() {
        return new LLIRConstant(ConstantType::Undef);
    }

    ConstantType getConstantType() const { return type_; }

    int64_t getIntValue() const { return intValue_; }
    double getFloatValue() const { return floatValue_; }

    bool isNull() const { return type_ == ConstantType::Null; }
    bool isUndef() const { return type_ == ConstantType::Undef; }
    bool isInteger() const { return type_ == ConstantType::Integer; }
    bool isFloat() const { return type_ == ConstantType::Float; }

    std::string toString() const override {
        switch (type_) {
            case ConstantType::Integer:
                return std::to_string(intValue_);
            case ConstantType::Float:
                return std::to_string(floatValue_);
            case ConstantType::Null:
                return "null";
            case ConstantType::Undef:
                return "undef";
            default:
                return "unknown";
        }
    }

    ValueType getValueType() const override {
        switch (type_) {
            case ConstantType::Integer:
                return ValueType::Integer;
            case ConstantType::Float:
                return ValueType::Float;
            case ConstantType::Null:
            case ConstantType::Undef:
                return ValueType::Pointer;
            default:
                return ValueType::Void;
        }
    }

private:
    ConstantType type_;
    union {
        int64_t intValue_;
        double floatValue_;
    };
};

/**
 * @brief LLIR变量（寄存器）
 */
class LLIRVariable : public LLIRValue {
public:
    LLIRVariable(const std::string& name, ValueType type, int id = -1)
        : name_(name), type_(type), id_(id) {}

    std::string getName() const { return name_; }
    ValueType getType() const { return type_; }
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    std::string toString() const override {
        if (id_ >= 0) {
            return "%" + name_ + "_" + std::to_string(id_);
        }
        return "%" + name_;
    }

    ValueType getValueType() const override { return type_; }

private:
    std::string name_;
    ValueType type_;
    int id_;
};

/**
 * @brief LLIR函数参数
 */
class LLIRArgument : public LLIRValue {
public:
    LLIRArgument(const std::string& name, ValueType type, int index)
        : name_(name), type_(type), index_(index) {}

    std::string getName() const { return name_; }
    ValueType getType() const { return type_; }
    int getIndex() const { return index_; }

    std::string toString() const override {
        return "%" + name_ + "_" + std::to_string(index_);
    }

    ValueType getValueType() const override { return type_; }

private:
    std::string name_;
    ValueType type_;
    int index_;
};

/**
 * @brief LLIR全局变量
 */
class LLIRGlobalVariable : public LLIRValue {
public:
    LLIRGlobalVariable(const std::string& name, ValueType type, bool isConst = false)
        : name_(name), type_(type), isConst_(isConst) {}

    std::string getName() const { return name_; }
    ValueType getType() const { return type_; }
    bool isConstant() const { return isConst_; }

    std::string toString() const override {
        return "@" + name_;
    }

    ValueType getValueType() const override { return type_; }

private:
    std::string name_;
    ValueType type_;
    bool isConst_;
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_LLIR_VALUE_H
