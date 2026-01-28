cmake_minimum_required(VERSION 3.20)
project(CVerifier VERSION 0.1.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出编译信息
message(STATUS "CVerifier v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 查找LLVM（可选）
find_package(LLVM 15 QUIET CONFIG)

if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
else()
    message(WARNING "LLVM not found. Clang frontend will be disabled.")
endif()

# 包含LLVM头文件（如果找到）
if(LLVM_FOUND)
    include_directories(${LLVM_INCLUDE_DIRS})
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
    add_definitions(${LLVM_DEFINITIONS_LIST})
endif()

# 查找Z3（可选）
find_package(Z3 4.12 QUIET)

if(Z3_FOUND)
    message(STATUS "Found Z3 ${Z3_VERSION}")
else()
    message(WARNING "Z3 not found. SMT solver will use simplified implementation.")
endif()

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# ============================================================================
# 核心库
# ============================================================================
add_library(cverifier-core
    # IR模块
    src/core/IR/LLIRModule.cpp
    src/core/IR/CFG.cpp

    # State模块
    src/core/State/SymbolicState.cpp
)

target_link_libraries(cverifier-core PUBLIC)

if(LLVM_FOUND)
    target_link_libraries(cverifier-core PUBLIC ${LLVM_LIBS})
endif()

if(Z3_FOUND)
    target_link_libraries(cverifier-core PUBLIC ${Z3_LIBRARIES})
    target_compile_definitions(cverifier-core PUBLIC HAVE_Z3)
endif()

target_include_directories(cverifier-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(LLVM_FOUND)
    target_include_directories(cverifier-core PUBLIC ${LLVM_INCLUDE_DIRS})
endif()

if(Z3_FOUND)
    target_include_directories(cverifier-core PUBLIC ${Z3_INCLUDE_DIRS})
endif()

# ============================================================================
# 前端库（仅在找到LLVM时构建）
# ============================================================================
if(LLVM_FOUND)
    add_library(cverifier-frontend
        src/frontend/clang/ClangParser.cpp
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

    target_compile_definitions(cverifier-frontend PUBLIC HAVE_LLVM)
endif()

# ============================================================================
# 分析器库
# ============================================================================
add_library(cverifier-analyzer
    # 符号执行
    src/analyzer/SymbolicExecution/Engine.cpp

    # 抽象解释
    src/analyzer/AbstractInterpretation/Interpreter.cpp

    # SMT求解器
    src/analyzer/Solver/Z3Solver.cpp

    # 漏洞检测器
    src/analyzer/Checkers/EnhancedCheckers.cpp
)

target_link_libraries(cverifier-analyzer PUBLIC
    cverifier-core
)

if(Z3_FOUND)
    target_link_libraries(cverifier-analyzer PUBLIC ${Z3_LIBRARIES})
    target_compile_definitions(cverifier-analyzer PUBLIC HAVE_Z3)
endif()

if(LLVM_FOUND)
    target_link_libraries(cverifier-analyzer PUBLIC ${LLVM_LIBS})
endif()

target_include_directories(cverifier-analyzer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(LLVM_FOUND)
    target_include_directories(cverifier-analyzer PUBLIC ${LLVM_INCLUDE_DIRS})
endif()

if(Z3_FOUND)
    target_include_directories(cverifier-analyzer PUBLIC ${Z3_INCLUDE_DIRS})
endif()

# ============================================================================
# 报告生成库（TODO: 待实现）
# ============================================================================
# add_library(cverifier-report
#     src/report/ReportGenerator.cpp
#     src/report/ErrorTrace.cpp
#     src/report/ConsoleReporter.cpp
#     src/report/SarifExporter.cpp
# )
#
# target_link_libraries(cverifier-report PUBLIC
#     cverifier-core
# )
#
# target_include_directories(cverifier-report PUBLIC
#     ${CMAKE_SOURCE_DIR}/include
# )

# ============================================================================
# 主程序
# ============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/tools/cverifier.cpp)
    add_executable(cverifier
        tools/cverifier.cpp
    )

    target_link_libraries(cverifier PRIVATE
        cverifier-core
        cverifier-analyzer
    )

    if(Z3_FOUND)
        target_link_libraries(cverifier PRIVATE ${Z3_LIBRARIES})
        target_compile_definitions(cverifier PRIVATE HAVE_Z3)
    endif()

    if(LLVM_FOUND)
        target_link_libraries(cverifier PRIVATE ${LLVM_LIBS})
    endif()

    target_include_directories(cverifier PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )

    if(LLVM_FOUND)
        target_include_directories(cverifier PRIVATE ${LLVM_INCLUDE_DIRS})
    endif()

    if(Z3_FOUND)
        target_include_directories(cverifier PRIVATE ${Z3_INCLUDE_DIRS})
    endif()
endif()

# ============================================================================
# Z3测试程序
# ============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/tools/test_z3.cpp)
    add_executable(test_z3
        tools/test_z3.cpp
    )

    target_link_libraries(test_z3 PRIVATE
        cverifier-core
        cverifier-analyzer
    )

    if(Z3_FOUND)
        target_link_libraries(test_z3 PRIVATE ${Z3_LIBRARIES})
        target_compile_definitions(test_z3 PRIVATE HAVE_Z3)
        target_include_directories(test_z3 PRIVATE ${Z3_INCLUDE_DIRS})
    endif()

    if(LLVM_FOUND)
        target_link_libraries(test_z3 PRIVATE ${LLVM_LIBS})
        target_include_directories(test_z3 PRIVATE ${LLVM_INCLUDE_DIRS})
    endif()

    target_include_directories(test_z3 PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
endif()

# ============================================================================
# Clang前端测试程序
# ============================================================================
if(LLVM_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/tools/test_clang.cpp)
    add_executable(test_clang
        tools/test_clang.cpp
    )

    target_link_libraries(test_clang PRIVATE
        cverifier-core
        cverifier-analyzer
        cverifier-frontend
    )

    if(Z3_FOUND)
        target_link_libraries(test_clang PRIVATE ${Z3_LIBRARIES})
        target_compile_definitions(test_clang PRIVATE HAVE_Z3)
        target_include_directories(test_clang PRIVATE ${Z3_INCLUDE_DIRS})
    endif()

    target_link_libraries(test_clang PRIVATE
        ${LLVM_LIBS}
        clangFrontend
        clangTooling
        clangAST
    )

    target_compile_definitions(test_clang PRIVATE HAVE_LLVM)

    target_include_directories(test_clang PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
    )
endif()

# ============================================================================
# 抽象解释测试程序
# ============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/tools/test_abstract.cpp)
    add_executable(test_abstract
        tools/test_abstract.cpp
    )

    target_link_libraries(test_abstract PRIVATE
        cverifier-core
        cverifier-analyzer
    )

    if(Z3_FOUND)
        target_link_libraries(test_abstract PRIVATE ${Z3_LIBRARIES})
        target_compile_definitions(test_abstract PRIVATE HAVE_Z3)
        target_include_directories(test_abstract PRIVATE ${Z3_INCLUDE_DIRS})
    endif()

    if(LLVM_FOUND)
        target_link_libraries(test_abstract PRIVATE ${LLVM_LIBS})
        target_compile_definitions(test_abstract PRIVATE HAVE_LLVM)
        target_include_directories(test_abstract PRIVATE ${LLVM_INCLUDE_DIRS})
    endif()

    target_include_directories(test_abstract PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
endif()

# ============================================================================
# 测试
# ============================================================================
enable_testing()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

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
if(LLVM_FOUND)
    message(STATUS "LLVM:           Found (${LLVM_PACKAGE_VERSION})")
else()
    message(STATUS "LLVM:           Not Found (Clang frontend disabled)")
endif()
if(Z3_FOUND)
    message(STATUS "Z3:             Found (${Z3_VERSION})")
else()
    message(STATUS "Z3:             Not Found (Using simplified SMT solver)")
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=========================================")
