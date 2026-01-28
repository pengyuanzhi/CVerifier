#ifndef CVERIFIER_ABSTRACT_INTERPRETER_H
#define CVERIFIER_ABSTRACT_INTERPRETER_H

#include "cverifier/LLIRModule.h"
#include "cverifier/SymbolicState.h"
#include "cverifier/CFG.h"
#include "cverifier/Core.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cverifier {
namespace core {

// ============================================================================
// 抽象值基类
// ============================================================================

/**
 * @brief 抽象值基类
 *
 * 抽象值表示程序中变量的抽象状态
 */
class AbstractValue {
public:
    virtual ~AbstractValue() = default;

    /**
     * @brief 获取类型
     */
    virtual ValueType getType() const = 0;

    /**
     * @brief 判断是否为顶（Top，表示所有可能值）
     */
    virtual bool isTop() const = 0;

    /**
     * @brief 判断是否为底（Bottom，表示无值）
     */
    virtual bool isBottom() const = 0;

    /**
     * @brief 转换为字符串
     */
    virtual std::string toString() const = 0;

    /**
     * @brief 克隆
     */
    virtual AbstractValue* clone() const = 0;

    /**
     * @brief 相等比较
     */
    virtual bool equals(const AbstractValue* other) const = 0;

    /**
     * @brief 创建顶值
     */
    static AbstractValue* createTop(ValueType type);

    /**
     * @brief 创建底值
     */
    static AbstractValue* createBottom(ValueType type);
};

// ============================================================================
// 抽象域基类
// ============================================================================

/**
 * @brief 抽象域基类
 *
 * 定义抽象域的格操作
 */
template<typename T>
class AbstractDomain {
public:
    virtual ~AbstractDomain() = default;

    /**
     * @brief 顶元素（最大元素）
     */
    virtual T top() const = 0;

    /**
     * @brief 底元素（最小元素）
     */
    virtual T bottom() const = 0;

    /**
     * @brief 求并（最小上界）
     */
    virtual T join(const T& a, const T& b) const = 0;

    /**
     * @brief 宽化操作（加速收敛）
     */
    virtual T widen(const T& a, const T& b) const = 0;

    /**
     * @brief 判断是否相等
     */
    virtual bool equals(const T& a, const T& b) const = 0;

    /**
     * @brief 判断是否达到不动点
     */
    virtual bool isStable(const T& before, const T& after) const {
        return equals(before, after);
    }
};

// ============================================================================
// 常量域（Constant Domain）
// ============================================================================

/**
 * @brief 常量抽象值
 */
class ConstantValue : public AbstractValue {
public:
    enum class ConstantType {
        Top,        // 未知（可能是任何值）
        Bottom,     // 无值
        Defined     // 已定义常量
    };

    ConstantValue() : type_(ConstantType::Top), valueType_(ValueType::Integer) {}
    ConstantValue(int64_t value) : type_(ConstantType::Defined), valueType_(ValueType::Integer), intValue_(value) {}
    ConstantValue(double value) : type_(ConstantType::Defined), valueType_(ValueType::Float), floatValue_(value) {}

    static ConstantValue* createTop(ValueType type) {
        auto* val = new ConstantValue();
        val->type_ = ConstantType::Top;
        val->valueType_ = type;
        return val;
    }

    static ConstantValue* createBottom(ValueType type) {
        auto* val = new ConstantValue();
        val->type_ = ConstantType::Bottom;
        val->valueType_ = type;
        return val;
    }

    ValueType getType() const override { return valueType_; }
    bool isTop() const override { return type_ == ConstantType::Top; }
    bool isBottom() const override { return type_ == ConstantType::Bottom; }
    bool isDefined() const { return type_ == ConstantType::Defined; }

    int64_t getIntValue() const { return intValue_; }
    double getFloatValue() const { return floatValue_; }

    std::string toString() const override {
        switch (type_) {
            case ConstantType::Top: return "⊤";
            case ConstantType::Bottom: return "⊥";
            case ConstantType::Defined:
                if (valueType_ == ValueType::Integer) {
                    return std::to_string(intValue_);
                } else {
                    return std::to_string(floatValue_);
                }
        }
        return "?";
    }

    AbstractValue* clone() const override {
        if (isDefined()) {
            if (valueType_ == ValueType::Integer) {
                return new ConstantValue(intValue_);
            } else {
                return new ConstantValue(floatValue_);
            }
        } else {
            auto* val = new ConstantValue();
            val->type_ = type_;
            val->valueType_ = valueType_;
            return val;
        }
    }

    bool equals(const AbstractValue* other) const override {
        auto* otherConst = dynamic_cast<const ConstantValue*>(other);
        if (!otherConst) return false;

        if (type_ != otherConst->type_) return false;
        if (valueType_ != otherConst->valueType_) return false;

        if (isDefined()) {
            if (valueType_ == ValueType::Integer) {
                return intValue_ == otherConst->intValue_;
            } else {
                return floatValue_ == otherConst->floatValue_;
            }
        }

        return true;
    }

private:
    ConstantType type_;
    ValueType valueType_;
    union {
        int64_t intValue_;
        double floatValue_;
    };
};

// ============================================================================
// 区间域（Interval Domain）
// ============================================================================

/**
 * @brief 区间抽象值
 *
 * 表示变量的取值范围 [low, high]
 */
class IntervalValue : public AbstractValue {
public:
    enum class IntervalType {
        Top,        // 未知（任意值）
        Bottom,     // 无值
        Bounded,    // 有界区间 [low, high]
        Unbounded   // 无界（只有一边有界）
    };

    IntervalValue() : type_(IntervalType::Top), valueType_(ValueType::Integer) {}
    IntervalValue(int64_t low, int64_t high)
        : type_(IntervalType::Bounded), valueType_(ValueType::Integer), lowInt_(low), highInt_(high) {}
    IntervalValue(double low, double high)
        : type_(IntervalType::Bounded), valueType_(ValueType::Float), lowFloat_(low), highFloat_(high) {}

    static IntervalValue* createTop(ValueType type) {
        auto* val = new IntervalValue();
        val->type_ = IntervalType::Top;
        val->valueType_ = type;
        return val;
    }

    static IntervalValue* createBottom(ValueType type) {
        auto* val = new IntervalValue();
        val->type_ = IntervalType::Bottom;
        val->valueType_ = type;
        return val;
    }

    ValueType getType() const override { return valueType_; }
    bool isTop() const override { return type_ == IntervalType::Top; }
    bool isBottom() const override { return type_ == IntervalType::Bottom; }
    bool isBounded() const { return type_ == IntervalType::Bounded; }

    int64_t getLowInt() const { return lowInt_; }
    int64_t getHighInt() const { return highInt_; }
    double getLowFloat() const { return lowFloat_; }
    double getHighFloat() const { return highFloat_; }

    /**
     * @brief 判断区间是否为单点（只有一个值）
     */
    bool isSingleton() const {
        if (type_ != IntervalType::Bounded) return false;
        if (valueType_ == ValueType::Integer) {
            return lowInt_ == highInt_;
        } else {
            return lowFloat_ == highFloat_;
        }
    }

    /**
     * @brief 判断是否包含特定值
     */
    bool contains(int64_t value) const {
        if (type_ == IntervalType::Top) return true;
        if (type_ == IntervalType::Bottom) return false;
        if (type_ == IntervalType::Bounded && valueType_ == ValueType::Integer) {
            return lowInt_ <= value && value <= highInt_;
        }
        return false;
    }

    std::string toString() const override {
        switch (type_) {
            case IntervalType::Top: return "[-∞, +∞]";
            case IntervalType::Bottom: return "⊥";
            case IntervalType::Bounded:
                if (valueType_ == ValueType::Integer) {
                    return "[" + std::to_string(lowInt_) + ", " + std::to_string(highInt_) + "]";
                } else {
                    return "[" + std::to_string(lowFloat_) + ", " + std::to_string(highFloat_) + "]";
                }
            case IntervalType::Unbounded:
                return "unbounded";
        }
        return "?";
    }

    AbstractValue* clone() const override {
        if (isBounded()) {
            if (valueType_ == ValueType::Integer) {
                return new IntervalValue(lowInt_, highInt_);
            } else {
                return new IntervalValue(lowFloat_, highFloat_);
            }
        } else {
            auto* val = new IntervalValue();
            val->type_ = type_;
            val->valueType_ = valueType_;
            return val;
        }
    }

    bool equals(const AbstractValue* other) const override {
        auto* otherInterval = dynamic_cast<const IntervalValue*>(other);
        if (!otherInterval) return false;

        if (type_ != otherInterval->type_) return false;
        if (valueType_ != otherInterval->valueType_) return false;

        if (isBounded()) {
            if (valueType_ == ValueType::Integer) {
                return lowInt_ == otherInterval->lowInt_ && highInt_ == otherInterval->highInt_;
            } else {
                return lowFloat_ == otherInterval->lowFloat_ && highFloat_ == otherInterval->highFloat_;
            }
        }

        return true;
    }

private:
    IntervalType type_;
    ValueType valueType_;
    union {
        struct {
            int64_t lowInt_;
            int64_t highInt_;
        };
        struct {
            double lowFloat_;
            double highFloat_;
        };
    };
};

// ============================================================================
// 抽象存储
// ============================================================================

/**
 * @brief 抽象存储
 *
 * 变量到抽象值的映射
 */
class AbstractStore {
public:
    AbstractStore() = default;
    ~AbstractStore();

    // 禁止拷贝
    AbstractStore(const AbstractStore&) = delete;
    AbstractStore& operator=(const AbstractStore&) = delete;

    /**
     * @brief 绑定变量到抽象值
     */
    void bind(const std::string& var, AbstractValue* value);

    /**
     * @brief 查找变量的抽象值
     */
    AbstractValue* lookup(const std::string& var) const;

    /**
     * @brief 合并两个存储（join操作）
     */
    AbstractStore* merge(const AbstractStore* other) const;

    /**
     * @brief 克隆
     */
    AbstractStore* clone() const;

    /**
     * @brief 转换为字符串
     */
    std::string toString() const;

    /**
     * @brief 判断是否相等或前驱
     */
    bool lessOrEqual(const AbstractStore* other) const;

private:
    std::unordered_map<std::string, AbstractValue*> bindings_;
};

// ============================================================================
// 转移函数
// ============================================================================

/**
 * @brief 转移函数
 *
 * 定义指令的抽象语义
 */
class TransferFunction {
public:
    virtual ~TransferFunction() = default;

    /**
     * @brief 执行转移函数
     * @param inst 当前指令
     * @param store 当前抽象存储
     * @return 新的抽象存储
     */
    virtual AbstractStore* transfer(
        LLIRInstruction* inst,
        const AbstractStore* store
    ) const = 0;
};

/**
 * @brief 区间域转移函数
 */
class IntervalTransferFunction : public TransferFunction {
public:
    AbstractStore* transfer(
        LLIRInstruction* inst,
        const AbstractStore* store
    ) const override;
};

// ============================================================================
// 不动点迭代器
// ============================================================================

/**
 * @brief 不动点迭代器
 *
 * 计算数据流分析的不动点
 */
class FixpointIterator {
public:
    FixpointIterator(CFG* cfg, TransferFunction* transferFunc);
    ~FixpointIterator();

    /**
     * @brief 计算不动点
     * @return 每个基本块的抽象存储
     */
    std::unordered_map<std::string, AbstractStore*> compute();

    /**
     * @brief 获取迭代次数
     */
    int getIterations() const { return iterations_; }

private:
    CFG* cfg_;
    TransferFunction* transferFunc_;
    int iterations_;

    /**
     * @brief 工作列表算法
     */
    std::unordered_map<std::string, AbstractStore*> worklistAlgorithm();
};

// ============================================================================
// 抽象解释引擎
// ============================================================================

/**
 * @brief 抽象解释引擎
 *
 * 抽象解释的主入口
 */
class AbstractInterpreter {
public:
    AbstractInterpreter(LLIRModule* module);
    ~AbstractInterpreter();

    /**
     * @brief 分析函数
     */
    void analyzeFunction(const std::string& functionName);

    /**
     * @brief 分析整个模块
     */
    void analyzeModule();

    /**
     * @brief 获取分析结果
     */
    const std::unordered_map<std::string, AbstractStore*>& getResults() const {
        return results_;
    }

    /**
     * @brief 设置抽象域类型
     */
    void setDomain(const std::string& domain) {
        domain_ = domain;
    }

    /**
     * @brief 获取统计信息
     */
    std::string getStatistics() const;

private:
    LLIRModule* module_;
    std::string domain_;
    std::unordered_map<std::string, AbstractStore*> results_;
    int functionsAnalyzed_;
    double analysisTime_;
};

} // namespace core
} // namespace cverifier

#endif // CVERIFIER_ABSTRACT_INTERPRETER_H
