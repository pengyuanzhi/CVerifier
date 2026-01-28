#ifndef CVERIFIER_CORE_H
#define CVERIFIER_CORE_H

/**
 * @file Core.h
 * @brief CVerifier核心模块公共接口
 *
 * 此文件包含所有核心模块的公共接口定义
 */

#include <memory>
#include <string>
#include <vector>

namespace cverifier {

// 版本信息
struct Version {
    static constexpr int MAJOR = 0;
    static constexpr int MINOR = 1;
    static constexpr int PATCH = 0;

    static std::string toString() {
        return std::to_string(MAJOR) + "." +
               std::to_string(MINOR) + "." +
               std::to_string(PATCH);
    }
};

// ============================================================================
// 前向声明
// ============================================================================

namespace core {
    class LLIRModule;
    class LLIRInstruction;
    class LLIRBasicBlock;
    class CFG;
}

namespace core {
    class SymbolicState;
    class SymbolicHeap;
    class PathConstraint;
}

// ============================================================================
// 公共类型定义
// ============================================================================

/// 值类型枚举
enum class ValueType {
    Integer,    ///< 整数
    Float,      ///< 浮点数
    Pointer,    ///< 指针
    Array,      ///< 数组
    Struct,     ///< 结构体
    Void        ///< 无类型
};

/// 漏洞类型枚举
enum class VulnerabilityType {
    BufferOverflow,      ///< 缓冲区溢出
    NullPointerDereference,  ///< 空指针解引用
    MemoryLeak,          ///< 内存泄漏
    IntegerOverflow,     ///< 整数溢出
    UseAfterFree,        ///< 释放后使用
    DoubleFree,          ///< 双重释放
    Unknown              ///< 未知类型
};

/// 严重程度枚举
enum class Severity {
    Low,        ///< 低
    Medium,     ///< 中
    High,       ///< 高
    Critical    ///< 严重
};

/// 漏洞报告结构
struct VulnerabilityReport {
    VulnerabilityType type;    ///< 漏洞类型
    Severity severity;          ///< 严重程度
    std::string file;           ///< 文件名
    int line;                   ///< 行号
    int column;                 ///< 列号
    std::string message;        ///< 描述信息
    std::string trace;          ///< 错误轨迹

    std::string toString() const;
};

/// 分析选项结构
struct AnalysisOptions {
    int maxDepth = 100;              ///< 最大探索深度
    int timeout = 300;               ///< 超时时间（秒）
    int maxStates = 10000;           ///< 最大状态数
    bool enableAbstraction = true;   ///< 启用抽象解释
    bool enableParallel = false;     ///< 启用并行分析
    int numThreads = 4;              ///< 并行线程数
};

/// 分析结果结构
struct AnalysisResult {
    bool success;                           ///< 是否成功
    int functionsAnalyzed;                  ///< 分析的函数数
    int pathsExplored;                      ///< 探索的路径数
    int vulnerabilitiesFound;               ///< 发现的漏洞数
    std::vector<VulnerabilityReport> reports;  ///< 漏洞报告列表
    double analysisTime;                    ///< 分析耗时（秒）
};

} // namespace cverifier

#endif // CVERIFIER_CORE_H
