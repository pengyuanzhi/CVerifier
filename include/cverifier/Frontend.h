#ifndef CVERIFIER_FRONTEND_H
#define CVERIFIER_FRONTEND_H

#include "cverifier/Core.h"
#include "cverifier/LLIRModule.h"
#include <memory>
#include <string>
#include <vector>

namespace cverifier {

// ============================================================================
// 前端接口
// ============================================================================

/**
 * @brief 解析选项
 */
struct ParseOptions {
    std::vector<std::string> includePaths;  ///< 包含路径
    std::vector<std::string> defines;       ///< 宏定义
    std::string standard = "c11";           ///< C标准
    bool optimize = false;                  ///< 是否优化
};

/**
 * @brief C代码解析器接口
 */
class IParser {
public:
    virtual ~IParser() = default;

    /**
     * @brief 解析C源文件
     * @param sourceFile 源文件路径
     * @param options 解析选项
     * @return 解析成功返回LLIR模块，失败返回nullptr
     */
    virtual std::unique_ptr<LLIRModule> parse(
        const std::string& sourceFile,
        const ParseOptions& options = ParseOptions()
    ) = 0;

    /**
     * @brief 解析C源代码字符串
     * @param sourceCode 源代码
     * @param options 解析选项
     * @return 解析成功返回LLIR模块，失败返回nullptr
     */
    virtual std::unique_ptr<LLIRModule> parseString(
        const std::string& sourceCode,
        const ParseOptions& options = ParseOptions()
    ) = 0;

    /**
     * @brief 获取最后的错误信息
     */
    virtual std::string getLastError() const = 0;
};

/**
 * @brief Clang解析器（基于LLVM/Clang）
 */
class ClangParser : public IParser {
public:
    ClangParser();
    ~ClangParser() override;

    std::unique_ptr<LLIRModule> parse(
        const std::string& sourceFile,
        const ParseOptions& options = ParseOptions()
    ) override;

    std::unique_ptr<LLIRModule> parseString(
        const std::string& sourceCode,
        const ParseOptions& options = ParseOptions()
    ) override;

    std::string getLastError() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief IR转换器接口（LLVM IR -> LLIR）
 */
class IIRConverter {
public:
    virtual ~IIRConverter() = default;

    /**
     * @brief 将LLVM IR转换为LLIR
     * @param llvmModule LLVM模块
     * @return 转换后的LLIR模块
     */
    virtual std::unique_ptr<LLIRModule> convert(void* llvmModule) = 0;
};

/**
 * @brief 默认IR转换器实现
 */
class DefaultIRConverter : public IIRConverter {
public:
    DefaultIRConverter();
    ~DefaultIRConverter() override;

    std::unique_ptr<LLIRModule> convert(void* llvmModule) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace cverifier

#endif // CVERIFIER_FRONTEND_H
