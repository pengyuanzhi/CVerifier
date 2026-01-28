#ifndef CVERIFIER_CLANG_PARSER_H
#define CVERIFIER_CLANG_PARSER_H

#include "cverifier/LLIRModule.h"
#include "cverifier/LLIRValue.h"
#include "cverifier/LLIRFactory.h"
#include "cverifier/Core.h"
#include <string>
#include <memory>
#include <unordered_map>

#ifdef HAVE_LLVM

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

namespace cverifier {
namespace frontend {

// 引入 core 命名空间的类型
using core::LLIRModule;
using core::LLIRFunction;
using core::LLIRBasicBlock;
using core::LLIRValue;
using core::LLIRInstruction;
using core::LLIRFactory;

// ValueType 在 cverifier 命名空间中，不在 core 中

// ============================================================================
// AST 到 LLIR 转换器
// ============================================================================

/**
 * @brief AST 到 LLIR 转换器
 *
 * 负责将 Clang AST 节点转换为 LLIR 指令
 */
class ASTToLLIRConverter {
public:
    ASTToLLIRConverter(LLIRModule* module)
        : module_(module), currentFunction_(nullptr), currentBB_(nullptr) {}

    /**
     * @brief 转换函数声明
     */
    LLIRFunction* convertFunctionDecl(clang::FunctionDecl* funcDecl);

    /**
     * @brief 转换函数体
     */
    void convertFunctionBody(clang::FunctionDecl* funcDecl, LLIRFunction* func);

    /**
     * @brief 转换语句
     */
    void convertStmt(clang::Stmt* stmt);

    /**
     * @brief 转换表达式
     */
    LLIRValue* convertExpr(clang::Expr* expr);

    /**
     * @brief 转换类型
     */
    ValueType convertType(clang::QualType qualType);

    /**
     * @brief 转换声明
     */
    void convertDecl(clang::Decl* decl);

private:
    /**
     * @brief 转换 if 语句
     */
    void convertIfStmt(clang::IfStmt* ifStmt);

    /**
     * @brief 转换 while 语句
     */
    void convertWhileStmt(clang::WhileStmt* whileStmt);

    /**
     * @brief 转换 for 语句
     */
    void convertForStmt(clang::ForStmt* forStmt);

    /**
     * @brief 转换 return 语句
     */
    void convertReturnStmt(clang::ReturnStmt* retStmt);

    /**
     * @brief 转换变量声明
     */
    void convertVarDecl(clang::VarDecl* varDecl);

    /**
     * @brief 转换二元操作
     */
    LLIRValue* convertBinaryOperator(clang::BinaryOperator* binOp);

    /**
     * @brief 转换一元操作
     */
    LLIRValue* convertUnaryOperator(clang::UnaryOperator* unaryOp);

    /**
     * @brief 转换数组访问
     */
    LLIRValue* convertArraySubscriptExpr(clang::ArraySubscriptExpr* arraySub);

    /**
     * @brief 转换函数调用
     */
    LLIRValue* convertCallExpr(clang::CallExpr* callExpr);

    /**
     * @brief 转换成员访问
     */
    LLIRValue* convertMemberExpr(clang::MemberExpr* memberExpr);

    /**
     * @brief 转换隐式转换
     */
    LLIRValue* convertImplicitCastExpr(clang::ImplicitCastExpr* castExpr);

    /**
     * @brief 创建新的基本块
     */
    LLIRBasicBlock* createBasicBlock(const std::string& name);

    /**
     * @brief 设置当前基本块
     */
    void setCurrentBasicBlock(LLIRBasicBlock* bb);

    /**
     * @brief 生成唯一的变量名
     */
    std::string freshVarName(const std::string& prefix = "var");

    LLIRModule* module_;
    LLIRFunction* currentFunction_;
    LLIRBasicBlock* currentBB_;

    int varCounter_ = 0;
    std::unordered_map<const clang::ValueDecl*, std::string> varMap_;
};

// ============================================================================
// AST 消费者
// ============================================================================

/**
 * @brief Clang AST 消费者
 *
 * 遍历 AST 并转换为 LLIR
 */
class CVerifierASTConsumer : public clang::ASTConsumer {
public:
    explicit CVerifierASTConsumer(LLIRModule* module)
        : converter_(module) {}

    void HandleTranslationUnit(clang::ASTContext& context) override;

private:
    ASTToLLIRConverter converter_;
};

// ============================================================================
// 前端动作
// ============================================================================

/**
 * @brief Clang 前端动作
 */
class CVerifierFrontendAction : public clang::ASTFrontendAction {
public:
    CVerifierFrontendAction(LLIRModule* module)
        : module_(module) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& CI,
        llvm::StringRef file
    ) override;

private:
    LLIRModule* module_;
};

// ============================================================================
// Clang 解析器
// ============================================================================

/**
 * @brief Clang 解析器
 *
 * 主入口点，用于解析 C 代码并生成 LLIR
 */
class ClangParser {
public:
    ClangParser();
    ~ClangParser();

    /**
     * @brief 解析源文件
     * @param filename 源文件路径
     * @param compileArgs 编译参数
     * @return LLIR 模块，失败返回 nullptr
     */
    LLIRModule* parseFile(
        const std::string& filename,
        const std::vector<std::string>& compileArgs = {}
    );

    /**
     * @brief 解析源代码字符串
     * @param code 源代码
     * @param filename 虚拟文件名
     * @param compileArgs 编译参数
     * @return LLIR 模块，失败返回 nullptr
     */
    LLIRModule* parseCode(
        const std::string& code,
        const std::string& filename = "input.c",
        const std::vector<std::string>& compileArgs = {}
    );

    /**
     * @brief 获取最后的错误信息
     */
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;
};

} // namespace frontend
} // namespace cverifier

#endif // HAVE_LLVM

#endif // CVERIFIER_CLANG_PARSER_H
