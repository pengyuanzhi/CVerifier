cmake_minimum_required(VERSION 3.20)
project(CVerifier VERSION 0.1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出编译信息
message(STATUS "CVerifier v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 查找LLVM
find_package(LLVM 15 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 包含LLVM头文件
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# 查找Z3
find_package(Z3 4.12 REQUIRED)

message(STATUS "Found Z3 ${Z3_VERSION}")

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# ============================================================================
# 核心库
# ============================================================================
add_library(cverifier-core
    # IR模块
    src/core/IR/LLIRModule.cpp
    src/core/IR/LLIRInstruction.cpp
    src/core/IR/LLIRBasicBlock.cpp
    src/core/IR/CFG.cpp

    # State模块
    src/core/State/SymbolicState.cpp
    src/core/State/SymbolicHeap.cpp
    src/core/State/PathConstraint.cpp

    # Utils模块
    src/core/Utils/Logger.cpp
    src/core/Utils/SourceLocation.cpp
)

target_link_libraries(cverifier-core PUBLIC
    ${LLVM_LIBS}
)

target_include_directories(cverifier-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)

# ============================================================================
# 前端库
# ============================================================================
add_library(cverifier-frontend
    src/frontend/clang/ClangParser.cpp
    src/frontend/clang/ASTConsumer.cpp
    src/frontend/clang/IRConverter.cpp
)

target_link_libraries(cverifier-frontend PUBLIC
    cverifier-core
    ${LLVM_LIBS}
    clangFrontend
    clangTooling
    clangAST
)

target_include_directories(cverifier-frontend PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)

# ============================================================================
# 分析器库
# ============================================================================
add_library(cverifier-analyzer
    # 符号执行
    src/analyzer/SymbolicExecution/Engine.cpp
    src/analyzer/SymbolicExecution/PathExplorer.cpp
    src/analyzer/SymbolicExecution/StateManager.cpp

    # 抽象解释
    src/analyzer/AbstractInterpretation/Interpreter.cpp
    src/analyzer/AbstractInterpretation/TransferFunctions.cpp
    src/analyzer/AbstractInterpretation/FixpointIterator.cpp
    src/analyzer/AbstractInterpretation/Domains/IntervalDomain.cpp
    src/analyzer/AbstractInterpretation/Domains/ConstantDomain.cpp

    # 漏洞检测器
    src/analyzer/Checkers/BufferOverflowChecker.cpp
    src/analyzer/Checkers/NullPointerChecker.cpp
    src/analyzer/Checkers/MemoryLeakChecker.cpp
    src/analyzer/Checkers/IntegerOverflowChecker.cpp
    src/analyzer/Checkers/CheckerRegistry.cpp

    # SMT求解器
    src/analyzer/Solver/Z3Solver.cpp
    src/analyzer/Solver/ConstraintBuilder.cpp
)

target_link_libraries(cverifier-analyzer PUBLIC
    cverifier-core
    cverifier-frontend
    ${Z3_LIBRARIES}
    ${LLVM_LIBS}
)

target_include_directories(cverifier-analyzer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
    ${Z3_INCLUDE_DIRS}
)

# ============================================================================
# 报告生成库
# ============================================================================
add_library(cverifier-report
    src/report/ReportGenerator.cpp
    src/report/ErrorTrace.cpp
    src/report/ConsoleReporter.cpp
    src/report/SarifExporter.cpp
)

target_link_libraries(cverifier-report PUBLIC
    cverifier-core
)

target_include_directories(cverifier-report PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# 主程序
# ============================================================================
add_executable(cverifier
    src/tools/cverifier.cpp
)

target_link_libraries(cverifier PRIVATE
    cverifier-core
    cverifier-frontend
    cverifier-analyzer
    cverifier-report
    ${Z3_LIBRARIES}
    ${LLVM_LIBS}
)

target_include_directories(cverifier PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)

# ============================================================================
# 测试
# ============================================================================
enable_testing()

add_subdirectory(tests)

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS cverifier
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

install(DIRECTORY configs/
    DESTINATION etc/cverifier
)

# ============================================================================
# 编译选项
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cverifier-core PRIVATE
        -Wall -Wextra -Wpedantic
    )
    target_compile_options(cverifier-frontend PRIVATE
        -Wall -Wextra -Wpedantic
    )
    target_compile_options(cverifier-analyzer PRIVATE
        -Wall -Wextra -Wpedantic
    )
elseif(MSVC)
    target_compile_options(cverifier-core PRIVATE
        /W4
    )
    target_compile_options(cverifier-frontend PRIVATE
        /W4
    )
    target_compile_options(cverifier-analyzer PRIVATE
        /W4
    )
endif()

# ============================================================================
# 打印配置信息
# ============================================================================
message(STATUS "=========================================")
message(STATUS "CVerifier Configuration Summary")
message(STATUS "=========================================")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "LLVM Version:   ${LLVM_PACKAGE_VERSION}")
message(STATUS "Z3 Version:     ${Z3_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=========================================")
