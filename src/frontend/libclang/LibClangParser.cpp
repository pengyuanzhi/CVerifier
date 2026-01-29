/**
 * @file LibClangParser.cpp
 * @brief 基于 libclang C API 的简化解析器实现
 */

#include "cverifier/LibClangParser.h"
#include "cverifier/LLIRFactory.h"
#include "cverifier/LLIRModule.h"
#include "cverifier/Utils.h"
#include <iostream>

#ifdef HAVE_LLVM

using namespace cverifier::core;

namespace cverifier {
namespace frontend {

LibClangParser::LibClangParser() {
    // 创建 libclang 索引
    index_ = clang_createIndex(0, 0);  // excludeDeclsFromPCH=0, displayDiagnostics=0

    if (!index_) {
        lastError_ = "Failed to create clang index";
        utils::Logger::error(lastError_);
    } else {
        utils::Logger::info("LibClang parser initialized");
    }
}

LibClangParser::~LibClangParser() {
    if (index_) {
        clang_disposeIndex(index_);
    }
}

LLIRModule* LibClangParser::parseFile(const std::string& filename) {
    utils::Logger::info("Parsing file: " + filename);

    if (!index_) {
        lastError_ = "Clang index not initialized";
        utils::Logger::error(lastError_);
        return nullptr;
    }

    // 解析翻译单元
    CXTranslationUnit tu = parseTranslationUnit(filename);
    if (!tu) {
        lastError_ = "Failed to parse translation unit";
        utils::Logger::error(lastError_);
        return nullptr;
    }

    // 创建 LLIR 模块
    auto* module = core::LLIRFactory::createModule(filename);

    // 遍历 AST 并生成 LLIR
    traverseAST(tu, module);

    // 清理
    clang_disposeTranslationUnit(tu);

    // 验证模块
    if (!module->validate()) {
        lastError_ = "Generated LLIR module is invalid";
        utils::Logger::error(lastError_);
        delete module;
        return nullptr;
    }

    utils::Logger::info("File parsed successfully");
    utils::Logger::info("Functions in module: " + std::to_string(module->getFunctions().size()));

    return module;
}

CXTranslationUnit LibClangParser::parseTranslationUnit(const std::string& filename) {
    // 准备编译参数
    std::vector<const char*> args;
    args.push_back("-I/usr/include");
    args.push_back("-I/usr/local/include");

    // 解析文件
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index_,
        filename.c_str(),
        args.data(),
        static_cast<int>(args.size()),
        nullptr,  // unsaved_files
        0,        // num_unsaved_files
        CXTranslationUnit_None
    );

    if (!tu) {
        utils::Logger::error("Unable to parse translation unit");
        return nullptr;
    }

    // 检查诊断信息
    unsigned numDiags = clang_getNumDiagnostics(tu);
    if (numDiags > 0) {
        for (unsigned i = 0; i < numDiags; ++i) {
            CXDiagnostic diag = clang_getDiagnostic(tu, i);
            CXString diagStr = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
            std::string diagText = clang_getCString(diagStr);

            // 只显示错误和警告
            CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
            if (severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal) {
                utils::Logger::error("Diagnostic: " + diagText);
            } else if (severity == CXDiagnostic_Warning) {
                utils::Logger::warning("Diagnostic: " + diagText);
            }

            clang_disposeString(diagStr);
            clang_disposeDiagnostic(diag);
        }
    }

    return tu;
}

void LibClangParser::traverseAST(CXTranslationUnit tu, LLIRModule* module) {
    if (!tu || !module) {
        return;
    }

    // 初始化访问
    CXCursor cursor = clang_getTranslationUnitCursor(tu);

    utils::Logger::debug("Traversing AST...");

    // 创建一个结构体来传递 parser 和 module
    struct ClientData {
        LibClangParser* parser;
        LLIRModule* module;
    };
    ClientData data = {this, module};

    // 递归遍历所有子节点
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) {
            ClientData* data = static_cast<ClientData*>(client_data);
            if (data && data->parser) {
                data->parser->processCursor(c, data->module);
            }
            (void)parent;  // 避免未使用参数警告
            return CXChildVisit_Continue;
        },
        &data
    );
}

void LibClangParser::processCursor(CXCursor cursor, LLIRModule* module) {
    if (!module) {
        return;
    }

    CXCursorKind kind = clang_getCursorKind(cursor);

    // 只处理顶层声明
    if (kind == CXCursor_FunctionDecl) {
        processFunctionDecl(cursor, module);
    }
}

void LibClangParser::processFunctionDecl(CXCursor cursor, LLIRModule* module) {
    // 获取函数名
    CXString nameCX = clang_getCursorSpelling(cursor);
    std::string funcName = clang_getCString(nameCX);
    clang_disposeString(nameCX);

    utils::Logger::debug("Processing function: " + funcName);

    // 检查是否有定义（不仅仅是声明）
    if (!clang_isCursorDefinition(cursor)) {
        utils::Logger::debug("  Skipping: no definition");
        return;
    }

    // 创建 LLIR 函数
    auto* func = core::LLIRFactory::createFunction(funcName);
    module->addFunction(func);

    // 创建入口基本块
    auto* entryBB = core::LLIRFactory::createBasicBlock("entry");
    func->addBasicBlock(entryBB);
    func->setEntryBlock(entryBB);

    // 获取函数参数
    int numArgs = clang_Cursor_getNumArguments(cursor);
    for (int i = 0; i < numArgs; ++i) {
        CXCursor arg = clang_Cursor_getArgument(cursor, i);
        CXString argNameCX = clang_getCursorSpelling(arg);
        std::string argName = clang_getCString(argNameCX);
        clang_disposeString(argNameCX);

        if (argName.empty()) {
            argName = "param_" + std::to_string(i);
        }

        utils::Logger::debug("  Parameter: " + argName);
    }

    // 简化实现：只记录函数存在，不转换函数体
    // 完整实现需要递归遍历函数体的 AST
    utils::Logger::debug("  Function added to module (body conversion not implemented)");
}

ValueType LibClangParser::getType(CXType type) {
    CXTypeKind kind = type.kind;

    switch (kind) {
        case CXType_Void:
            return ValueType::Void;
        case CXType_Bool:
        case CXType_Char_U:
        case CXType_Char_S:
        case CXType_SChar:
        case CXType_UChar:
        case CXType_Short:
        case CXType_UShort:
        case CXType_Int:
        case CXType_UInt:
        case CXType_Long:
        case CXType_ULong:
        case CXType_LongLong:
        case CXType_ULongLong:
            return ValueType::Integer;
        case CXType_Float:
        case CXType_Double:
        case CXType_LongDouble:
            return ValueType::Float;
        case CXType_Pointer:
            return ValueType::Pointer;
        // CXType_Array 可能在某些版本的 libclang 中不存在
        // case CXType_Array:
        //     return ValueType::Array;
        case CXType_Record:
            return ValueType::Struct;
        default:
            // 对于未知类型，默认使用整数类型
            return ValueType::Integer;
    }
}

} // namespace frontend
} // namespace cverifier

#endif // HAVE_LLVM
