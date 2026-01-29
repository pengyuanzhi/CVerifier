#ifndef CVERIFIER_LIBCLANG_PARSER_H
#define CVERIFIER_LIBCLANG_PARSER_H

#include "cverifier/LLIRModule.h"
#include "cverifier/Core.h"
#include <string>
#include <vector>

#ifdef HAVE_LLVM

// 使用 libclang 的 C 接口
#include <clang-c/Index.h>

namespace cverifier {
namespace frontend {

/**
 * @brief 基于 libclang C API 的简化解析器
 *
 * 使用 libclang 的 C 接口来解析 C 代码，比完整的 Clang C++ API
 * 更容易集成，但功能相对有限。
 */
class LibClangParser {
public:
    LibClangParser();
    ~LibClangParser();

    /**
     * @brief 解析 C 源文件
     * @param filename 源文件路径
     * @return LLIR 模块，失败返回 nullptr
     */
    LLIRModule* parseFile(const std::string& filename);

    /**
     * @brief 获取最后的错误信息
     */
    std::string getLastError() const { return lastError_; }

private:
    /**
     * @brief 解析翻译单元
     */
    CXTranslationUnit parseTranslationUnit(const std::string& filename);

    /**
     * @brief 遍历 AST 并生成 LLIR
     */
    void traverseAST(CXTranslationUnit tu, LLIRModule* module);

    /**
     * @brief 处理光标
     */
    void processCursor(CXCursor cursor, LLIRModule* module);

    /**
     * @brief 处理函数声明
     */
    void processFunctionDecl(CXCursor cursor, LLIRModule* module);

    /**
     * @brief 获取光标对应的类型
     */
    ValueType getType(CXType type);

    CXIndex index_;
    std::string lastError_;
};

} // namespace frontend
} // namespace cverifier

#endif // HAVE_LLVM

#endif // CVERIFIER_LIBCLANG_PARSER_H
