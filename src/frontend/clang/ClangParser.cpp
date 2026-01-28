/**
 * @file ClangParser.cpp
 * @brief Clang 前端实现 - AST 到 LLIR 的转换
 */

#include "cverifier/ClangParser.h"
#include "cverifier/LLIRFactory.h"
#include "cverifier/Utils.h"
#include "llvm/Support/raw_ostream.h"

#ifdef HAVE_LLVM

namespace cverifier {
namespace frontend {

// ============================================================================
// ASTToLLIRConverter 实现
// ============================================================================

LLIRFunction* ASTToLLIRConverter::convertFunctionDecl(clang::FunctionDecl* funcDecl) {
    if (!funcDecl || !funcDecl->hasBody()) {
        return nullptr;
    }

    // 获取函数名
    std::string funcName = funcDecl->getNameInfo().getName().str();

    utils::Logger::debug("Converting function: " + funcName);

    // 创建 LLIR 函数
    auto* func = LLIRFactory::createFunction(funcName);
    module_->addFunction(func);

    // 保存当前函数
    currentFunction_ = func;

    // 创建入口基本块
    auto* entryBB = createBasicBlock("entry");
    func->addBasicBlock(entryBB);
    func->setEntryBlock(entryBB);
    setCurrentBasicBlock(entryBB);

    // 转换函数参数
    int paramIndex = 0;
    for (auto* param : funcDecl->parameters()) {
        std::string paramName = param->getName().str();
        if (paramName.empty()) {
            paramName = "param_" + std::to_string(paramIndex);
        }

        // 记录参数映射
        varMap_[param] = paramName;

        // 创建 LLIR 参数（实际在LLIR中用变量表示）
        auto* paramVar = LLIRFactory::createVariable(
            paramName,
            convertType(param->getType()),
            paramIndex
        );

        paramIndex++;
    }

    // 转换函数体
    convertFunctionBody(funcDecl, func);

    // 清除当前函数
    currentFunction_ = nullptr;
    varMap_.clear();

    return func;
}

void ASTToLLIRConverter::convertFunctionBody(
    clang::FunctionDecl* funcDecl,
    LLIRFunction* func
) {
    if (!funcDecl->hasBody()) {
        return;
    }

    clang::Stmt* body = funcDecl->getBody();
    if (!body) {
        return;
    }

    // 如果是复合语句（函数体），转换其中的每个语句
    if (auto* compoundStmt = clang::dyn_cast<clang::CompoundStmt>(body)) {
        for (auto* stmt : compoundStmt->body()) {
            convertStmt(stmt);
        }
    } else {
        // 单个语句
        convertStmt(body);
    }
}

void ASTToLLIRConverter::convertStmt(clang::Stmt* stmt) {
    if (!stmt) {
        return;
    }

    // 获取源代码位置
    SourceLocation loc;
    auto& sm = currentFunction_->getModule()->getSourceManager();
    auto clangLoc = stmt->getBeginLoc();
    if (clangLoc.isValid()) {
        loc.file = sm.getFilename(clangLoc).str();
        loc.line = sm.getSpellingLineNumber(clangLoc);
        loc.column = sm.getSpellingColumnNumber(clangLoc);
    }

    // 根据语句类型转换
    if (auto* ifStmt = clang::dyn_cast<clang::IfStmt>(stmt)) {
        convertIfStmt(ifStmt);
    } else if (auto* whileStmt = clang::dyn_cast<clang::WhileStmt>(stmt)) {
        convertWhileStmt(whileStmt);
    } else if (auto* forStmt = clang::dyn_cast<clang::ForStmt>(stmt)) {
        convertForStmt(forStmt);
    } else if (auto* retStmt = clang::dyn_cast<clang::ReturnStmt>(stmt)) {
        convertReturnStmt(retStmt);
    } else if (auto* compoundStmt = clang::dyn_cast<clang::CompoundStmt>(stmt)) {
        // 复合语句：逐个转换子语句
        for (auto* s : compoundStmt->body()) {
            convertStmt(s);
        }
    } else if (auto* declStmt = clang::dyn_cast<clang::DeclStmt>(stmt)) {
        // 声明语句
        for (auto* decl : declStmt->decls()) {
            convertDecl(decl);
        }
    } else if (auto* exprStmt = clang::dyn_cast<clang::Expr>(stmt)) {
        // 表达式语句：转换表达式并丢弃结果
        convertExpr(exprStmt);
    }
}

LLIRValue* ASTToLLIRConverter::convertExpr(clang::Expr* expr) {
    if (!expr) {
        return nullptr;
    }

    // 处理隐式转换
    if (auto* implicitCast = clang::dyn_cast<clang::ImplicitCastExpr>(expr)) {
        return convertImplicitCastExpr(implicitCast);
    }

    // 二元操作
    if (auto* binOp = clang::dyn_cast<clang::BinaryOperator>(expr)) {
        return convertBinaryOperator(binOp);
    }

    // 一元操作
    if (auto* unaryOp = clang::dyn_cast<clang::UnaryOperator>(expr)) {
        return convertUnaryOperator(unaryOp);
    }

    // 数组访问
    if (auto* arraySub = clang::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
        return convertArraySubscriptExpr(arraySub);
    }

    // 函数调用
    if (auto* callExpr = clang::dyn_cast<clang::CallExpr>(expr)) {
        return convertCallExpr(callExpr);
    }

    // 成员访问
    if (auto* memberExpr = clang::dyn_cast<clang::MemberExpr>(expr)) {
        return convertMemberExpr(memberExpr);
    }

    // 整数常量
    if (auto* intLiteral = clang::dyn_cast<clang::IntegerLiteral>(expr)) {
        llvm::APInt value = intLiteral->getValue();
        return LLIRFactory::createIntConstant(value.getLimitedValue());
    }

    // 浮点常量
    if (auto* floatLiteral = clang::dyn_cast<clang::FloatingLiteral>(expr)) {
        return LLIRFactory::createFloatConstant(floatLiteral->getValueAsApproximateDouble());
    }

    // 字符常量
    if (auto* charLiteral = clang::dyn_cast<clang::CharacterLiteral>(expr)) {
        return LLIRFactory::createIntConstant(charLiteral->getValue());
    }

    // 声明引用（变量引用）
    if (auto* declRef = clang::dyn_cast<clang::DeclRefExpr>(expr)) {
        if (auto* varDecl = clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
            auto it = varMap_.find(varDecl);
            if (it != varMap_.end()) {
                // 返回已存在的变量
                return LLIRFactory::createVariable(
                    it->second,
                    convertType(varDecl->getType())
                );
            } else {
                // 创建新的变量引用
                std::string varName = varDecl->getName().str();
                if (varName.empty()) {
                    varName = freshVarName("anon");
                }
                varMap_[varDecl] = varName;
                return LLIRFactory::createVariable(
                    varName,
                    convertType(varDecl->getType())
                );
            }
        }
    }

    // 括号表达式
    if (auto* parenExpr = clang::dyn_cast<clang::ParenExpr>(expr)) {
        return convertExpr(parenExpr->getSubExpr());
    }

    utils::Logger::warning("Unsupported expression type: " + std::string(expr->getStmtClassName()));
    return nullptr;
}

ValueType ASTToLLIRConverter::convertType(clang::QualType qualType) {
    const clang::Type* type = qualType.getTypePtr();

    if (type->isIntegerType()) {
        if (type->isSignedIntegerType()) {
            return ValueType::Integer;
        } else {
            return ValueType::Integer;  // 无符号也用Integer表示
        }
    } else if (type->isFloatingType()) {
        return ValueType::Float;
    } else if (type->isPointerType()) {
        return ValueType::Pointer;
    } else if (type->isArrayType()) {
        return ValueType::Array;
    } else if (type->isStructureType() || type->isUnionType()) {
        return ValueType::Struct;
    } else if (type->isVoidType()) {
        return ValueType::Void;
    }

    return ValueType::Void;  // 默认
}

void ASTToLLIRConverter::convertDecl(clang::Decl* decl) {
    if (!decl) {
        return;
    }

    if (auto* varDecl = clang::dyn_cast<clang::VarDecl>(decl)) {
        convertVarDecl(varDecl);
    }
}

void ASTToLLIRConverter::convertIfStmt(clang::IfStmt* ifStmt) {
    if (!ifStmt) {
        return;
    }

    utils::Logger::debug("Converting if statement");

    // 转换条件
    LLIRValue* condition = convertExpr(ifStmt->getCond());

    // 创建 then 和 else 基本块
    auto* thenBB = createBasicBlock("if.then");
    auto* elseBB = createBasicBlock("if.else");
    auto* mergeBB = createBasicBlock("if.end");

    // 创建条件分支指令
    auto* brInst = LLIRFactory::createConditionalBr(condition, thenBB, elseBB,
        ifStmt->getIfLoc().printToString(currentFunction_->getModule()->getSourceManager()));
    currentBB_->addInstruction(brInst);

    currentFunction_->addBasicBlock(thenBB);
    currentFunction_->addBasicBlock(elseBB);
    currentFunction_->addBasicBlock(mergeBB);

    // 转换 then 分支
    setCurrentBasicBlock(thenBB);
    convertStmt(ifStmt->getThen());

    // then 分支结束后跳转到 merge
    if (currentBB_->getInstructions().empty() ||
        currentBB_->getInstructions().back()->getType() != LLIRInstructionType::Br) {
        auto* thenMerge = LLIRFactory::createBr(mergeBB);
        currentBB_->addInstruction(thenMerge);
    }

    // 转换 else 分支（如果存在）
    setCurrentBasicBlock(elseBB);
    if (ifStmt->getElse()) {
        convertStmt(ifStmt->getElse());
    }

    // else 分支结束后跳转到 merge
    if (currentBB_->getInstructions().empty() ||
        currentBB_->getInstructions().back()->getType() != LLIRInstructionType::Br) {
        auto* elseMerge = LLIRFactory::createBr(mergeBB);
        currentBB_->addInstruction(elseMerge);
    }

    // 设置 merge 为当前基本块
    setCurrentBasicBlock(mergeBB);
}

void ASTToLLIRConverter::convertWhileStmt(clang::WhileStmt* whileStmt) {
    if (!whileStmt) {
        return;
    }

    utils::Logger::debug("Converting while statement");

    // 创建循环基本块
    auto* condBB = createBasicBlock("while.cond");
    auto* bodyBB = createBasicBlock("while.body");
    auto* endBB = createBasicBlock("while.end");

    // 当前基本块跳转到条件检查块
    auto* toCond = LLIRFactory::createBr(condBB);
    currentBB_->addInstruction(toCond);

    currentFunction_->addBasicBlock(condBB);
    currentFunction_->addBasicBlock(bodyBB);
    currentFunction_->addBasicBlock(endBB);

    // 转换条件
    setCurrentBasicBlock(condBB);
    LLIRValue* condition = convertExpr(whileStmt->getCond());

    // 创建条件分支
    auto* brInst = LLIRFactory::createConditionalBr(condition, bodyBB, endBB,
        whileStmt->getWhileLoc().printToString(currentFunction_->getModule()->getSourceManager()));
    currentBB_->addInstruction(brInst);

    // 转换循环体
    setCurrentBasicBlock(bodyBB);
    convertStmt(whileStmt->getBody());

    // 循环体结束后跳回条件检查
    if (currentBB_->getInstructions().empty() ||
        currentBB_->getInstructions().back()->getType() != LLIRInstructionType::Br) {
        auto* backToCond = LLIRFactory::createBr(condBB);
        currentBB_->addInstruction(backToCond);
    }

    // 设置 end 为当前基本块
    setCurrentBasicBlock(endBB);
}

void ASTToLLIRConverter::convertForStmt(clang::ForStmt* forStmt) {
    if (!forStmt) {
        return;
    }

    utils::Logger::debug("Converting for statement");

    // 转换初始化
    if (forStmt->getInit()) {
        convertStmt(forStmt->getInit());
    }

    // 创建循环基本块
    auto* condBB = createBasicBlock("for.cond");
    auto* bodyBB = createBasicBlock("for.body");
    auto* incBB = createBasicBlock("for.inc");
    auto* endBB = createBasicBlock("for.end");

    // 跳转到条件检查
    auto* toCond = LLIRFactory::createBr(condBB);
    currentBB_->addInstruction(toCond);

    currentFunction_->addBasicBlock(condBB);
    currentFunction_->addBasicBlock(bodyBB);
    currentFunction_->addBasicBlock(incBB);
    currentFunction_->addBasicBlock(endBB);

    // 转换条件
    setCurrentBasicBlock(condBB);
    if (forStmt->getCond()) {
        LLIRValue* condition = convertExpr(forStmt->getCond());
        auto* brInst = LLIRFactory::createConditionalBr(condition, bodyBB, endBB,
            forStmt->getForLoc().printToString(currentFunction_->getModule()->getSourceManager()));
        currentBB_->addInstruction(brInst);
    } else {
        // 无条件：总是进入循环体
        auto* brInst = LLIRFactory::createBr(bodyBB);
        currentBB_->addInstruction(brInst);
    }

    // 转换循环体
    setCurrentBasicBlock(bodyBB);
    convertStmt(forStmt->getBody());

    // 循环体结束后跳到增量
    if (currentBB_->getInstructions().empty() ||
        currentBB_->getInstructions().back()->getType() != LLIRInstructionType::Br) {
        auto* toInc = LLIRFactory::createBr(incBB);
        currentBB_->addInstruction(toInc);
    }

    // 转换增量
    setCurrentBasicBlock(incBB);
    if (forStmt->getInc()) {
        convertExpr(forStmt->getInc());
    }

    // 增量后跳回条件检查
    auto* backToCond = LLIRFactory::createBr(condBB);
    currentBB_->addInstruction(backToCond);

    // 设置 end 为当前基本块
    setCurrentBasicBlock(endBB);
}

void ASTToLLIRConverter::convertReturnStmt(clang::ReturnStmt* retStmt) {
    if (!retStmt) {
        return;
    }

    utils::Logger::debug("Converting return statement");

    LLIRValue* returnValue = nullptr;
    if (retStmt->getRetValue()) {
        returnValue = convertExpr(retStmt->getRetValue());
    }

    auto* retInst = LLIRFactory::createRet(returnValue,
        retStmt->getReturnLoc().printToString(currentFunction_->getModule()->getSourceManager()));
    currentBB_->addInstruction(retInst);
}

void ASTToLLIRConverter::convertVarDecl(clang::VarDecl* varDecl) {
    if (!varDecl) {
        return;
    }

    std::string varName = varDecl->getName().str();
    if (varName.empty()) {
        varName = freshVarName("anon");
    }

    utils::Logger::debug("Converting variable declaration: " + varName);

    // 记录变量映射
    varMap_[varDecl] = varName;

    // 如果有初始化表达式，转换它
    if (varDecl->hasInit()) {
        LLIRValue* initValue = convertExpr(varDecl->getInit());

        // 在 LLIR 中，变量赋值是通过符号状态管理的
        // 这里我们不需要显式创建指令，只需要记录变量名
    }
}

LLIRValue* ASTToLLIRConverter::convertBinaryOperator(clang::BinaryOperator* binOp) {
    if (!binOp) {
        return nullptr;
    }

    LLIRValue* left = convertExpr(binOp->getLHS());
    LLIRValue* right = convertExpr(binOp->getRHS());

    if (!left || !right) {
        return nullptr;
    }

    // 获取操作码
    auto opcode = binOp->getOpcode();

    // 创建对应的 LLIR 指令
    switch (opcode) {
        case clang::BO_Add:
            return LLIRFactory::createAdd(left, right);
        case clang::BO_Sub:
            return LLIRFactory::createSub(left, right);
        case clang::BO_Mul:
            return LLIRFactory::createMul(left, right);
        case clang::BO_Div:
            return LLIRFactory::createDiv(left, right);
        case clang::BO_Rem:
            return LLIRFactory::createRem(left, right);
        case clang::BO_EQ:
            return LLIRFactory::createICmp(left, right);
        case clang::BO_NE:
            return LLIRFactory::createICmp(left, right);
        case clang::BO_LT:
            return LLIRFactory::createICmp(left, right);
        case clang::BO_GT:
            return LLIRFactory::createICmp(left, right);
        case clang::BO_LE:
            return LLIRFactory::createICmp(left, right);
        case clang::BO_GE:
            return LLIRFactory::createICmp(left, right);
        case clang::BO_LAnd:
            return LLIRFactory::createAnd(left, right);
        case clang::BO_LOr:
            return LLIRFactory::createOr(left, right);
        case clang::BO_Assign: {
            // 赋值操作：需要特殊处理
            // 获取左值的变量名
            if (auto* declRef = clang::dyn_cast<clang::DeclRefExpr>(binOp->getLHS())) {
                if (auto* varDecl = clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
                    varMap_[varDecl] = freshVarName("assign");
                }
            }
            return right;
        }
        default:
            utils::Logger::warning("Unsupported binary operator: " + std::string(binOp->getOpcodeStr()));
            return nullptr;
    }
}

LLIRValue* ASTToLLIRConverter::convertUnaryOperator(clang::UnaryOperator* unaryOp) {
    if (!unaryOp) {
        return nullptr;
    }

    LLIRValue* operand = convertExpr(unaryOp->getSubExpr());
    if (!operand) {
        return nullptr;
    }

    auto opcode = unaryOp->getOpcode();

    switch (opcode) {
        case clang::UO_Minus:
            return LLIRFactory::createSub(
                LLIRFactory::createIntConstant(0),
                operand
            );
        case clang::UO_Not:
        case clang::UO_LNot:
            // 逻辑非：需要实现
            return operand;
        case clang::UO_Deref: {
            // 解引用操作：load
            auto* loadInst = LLIRFactory::createLoad(operand,
                unaryOp->getExprLoc().printToString(currentFunction_->getModule()->getSourceManager()));
            currentBB_->addInstruction(loadInst);
            return operand;
        }
        case clang::UO_AddrOf: {
            // 取地址操作
            return operand;
        }
        default:
            utils::Logger::warning("Unsupported unary operator");
            return nullptr;
    }
}

LLIRValue* ASTToLLIRConverter::convertArraySubscriptExpr(clang::ArraySubscriptExpr* arraySub) {
    if (!arraySub) {
        return nullptr;
    }

    LLIRValue* base = convertExpr(arraySub->getBase());
    LLIRValue* index = convertExpr(arraySub->getIdx());

    if (!base || !index) {
        return nullptr;
    }

    // 创建 GEP 指令
    auto* gepInst = LLIRFactory::createGetElementPtr(base, index,
        arraySub->getExprLoc().printToString(currentFunction_->getModule()->getSourceManager()));
    currentBB_->addInstruction(gepInst);

    // 创建 load 指令（获取数组元素）
    auto* loadInst = LLIRFactory::createLoad(gepInst,
        arraySub->getExprLoc().printToString(currentFunction_->getModule()->getSourceManager()));
    currentBB_->addInstruction(loadInst);

    return base;
}

LLIRValue* ASTToLLIRConverter::convertCallExpr(clang::CallExpr* callExpr) {
    if (!callExpr) {
        return nullptr;
    }

    // 获取被调函数
    std::string functionName;
    if (auto* declRef = clang::dyn_cast<clang::DeclRefExpr>(callExpr->getCallee())) {
        if (auto* funcDecl = clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
            functionName = funcDecl->getNameInfo().getName().str();
        }
    }

    if (functionName.empty()) {
        functionName = "unknown_function";
    }

    // 转换参数
    std::vector<LLIRValue*> args;
    for (auto* arg : callExpr->arguments()) {
        LLIRValue* argValue = convertExpr(arg);
        if (argValue) {
            args.push_back(argValue);
        }
    }

    // 创建 call 指令
    auto* callInst = LLIRFactory::createCall(functionName, args,
        callExpr->getExprLoc().printToString(currentFunction_->getModule()->getSourceManager()));
    currentBB_->addInstruction(callInst);

    return LLIRFactory::createVariable(
        freshVarName("call_result"),
        ValueType::Integer
    );
}

LLIRValue* ASTToLLIRConverter::convertMemberExpr(clang::MemberExpr* memberExpr) {
    if (!memberExpr) {
        return nullptr;
    }

    // 成员访问：简化处理，返回基对象
    LLIRValue* base = convertExpr(memberExpr->getBase());
    return base;
}

LLIRValue* ASTToLLIRConverter::convertImplicitCastExpr(clang::ImplicitCastExpr* castExpr) {
    if (!castExpr) {
        return nullptr;
    }

    // 递归转换子表达式
    return convertExpr(castExpr->getSubExpr());
}

LLIRBasicBlock* ASTToLLIRConverter::createBasicBlock(const std::string& name) {
    std::string uniqueName = name;
    int suffix = 0;

    // 确保名称唯一
    while (currentFunction_->getBasicBlock(uniqueName) != nullptr) {
        uniqueName = name + "_" + std::to_string(suffix++);
    }

    return LLIRFactory::createBasicBlock(uniqueName);
}

void ASTToLLIRConverter::setCurrentBasicBlock(LLIRBasicBlock* bb) {
    currentBB_ = bb;
}

std::string ASTToLLIRConverter::freshVarName(const std::string& prefix) {
    return prefix + "_" + std::to_string(varCounter_++);
}

// ============================================================================
// CVerifierASTConsumer 实现
// ============================================================================

void CVerifierASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
    utils::Logger::info("Processing translation unit");

    // 遍历所有函数声明
    for (auto* decl : context.getTranslationUnitDecl()->decls()) {
        if (auto* funcDecl = clang::dyn_cast<clang::FunctionDecl>(decl)) {
            if (funcDecl->hasBody()) {
                converter_.convertFunctionDecl(funcDecl);
            }
        }
    }
}

// ============================================================================
// CVerifierFrontendAction 实现
// ============================================================================

std::unique_ptr<clang::ASTConsumer> CVerifierFrontendAction::CreateASTConsumer(
    clang::CompilerInstance& CI,
    llvm::StringRef file
) {
    return std::make_unique<CVerifierASTConsumer>(module_);
}

// ============================================================================
// ClangParser 实现
// ============================================================================

ClangParser::ClangParser() {
    utils::Logger::info("Clang parser initialized");
}

ClangParser::~ClangParser() {
    // 清理
}

LLIRModule* ClangParser::parseFile(
    const std::string& filename,
    const std::vector<std::string>& compileArgs
) {
    utils::Logger::info("Parsing file: " + filename);

    // 创建 LLIR 模块
    auto* module = LLIRFactory::createModule(filename);

    // 准备编译参数
    std::vector<std::string> args;
    args.push_back("cverifier");
    args.push_back(filename);

    // 添加编译参数
    for (const auto& arg : compileArgs) {
        args.push_back(arg);
    }

    // 添加默认参数
    args.push_back("-I/usr/include");
    args.push_back("-I/usr/local/include");

    // 创建前端动作
    auto* action = new CVerifierFrontendAction(module);

    // 运行 Clang 工具
    llvm::Expected<clang::tooling::CommonOptionsParser> optParser =
        clang::tooling::CommonOptionsParser::create(
            args.size(),
            const_cast<char**>(args.data()),
            clang::tooling::CommonOptionsParser::HelpMessage
        );

    if (!optParser) {
        lastError_ = "Failed to create options parser";
        utils::Logger::error(lastError_);
        delete module;
        delete action;
        return nullptr;
    }

    clang::tooling::ClangTool tool(optParser->getCompilations(),
                                   optParser->getSourcePathList());

    int result = tool.run(clang::tooling::newFrontendActionFactory(action).get());

    delete action;

    if (result != 0) {
        lastError_ = "Clang tool execution failed";
        utils::Logger::error(lastError_);
        delete module;
        return nullptr;
    }

    // 验证模块
    if (!module->validate()) {
        lastError_ = "Generated LLIR module is invalid";
        utils::Logger::error(lastError_);
        delete module;
        return nullptr;
    }

    utils::Logger::info("File parsed successfully");
    return module;
}

LLIRModule* ClangParser::parseCode(
    const std::string& code,
    const std::string& filename,
    const std::vector<std::string>& compileArgs
) {
    utils::Logger::info("Parsing code string");

    // 创建临时文件
    // 实际实现中，可以使用 Clang 的内存缓冲区
    // 这里简化处理：将代码写入临时文件

    return nullptr;
}

} // namespace frontend
} // namespace cverifier

#endif // HAVE_LLVM
