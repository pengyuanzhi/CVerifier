# 测试配置

# 查找Google Test（可选）
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Found Google Test, building unit tests")

    # 单元测试
    add_executable(cverifier-unit-tests
        unit/IR/TestLLIRModule.cpp
        unit/State/TestSymbolicState.cpp
        unit/Solver/TestZ3Solver.cpp
        unit/Checkers/TestBufferOverflowChecker.cpp
    )

    target_link_libraries(cverifier-unit-tests
        cverifier-core
        cverifier-analyzer
        GTest::GTest
        GTest::Main
    )

    add_test(NAME UnitTests COMMAND cverifier-unit-tests)
else()
    message(WARNING "Google Test not found, unit tests will be disabled")
    message(WARNING "To enable tests, install Google Test:")
    message(WARNING "  Ubuntu: sudo apt install libgtest-dev")
    message(WARNING "  macOS:  brew install googletest")
    message(WARNING "  Windows: vcpkg install gtest")
endif()

# 集成测试（不需要GTest）
function(add_integration_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} cverifier)

    add_test(NAME ${name} COMMAND ${name})
endfunction()

# 添加集成测试
add_integration_test(test-buffer-overflow integration/buffer-overflow/test_buffer_overflow.c)
add_integration_test(test-null-pointer integration/null-pointer/test_null_pointer.c)
add_integration_test(test-memory-leak integration/memory-leak/test_memory_leak.c)
