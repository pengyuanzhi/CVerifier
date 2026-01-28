# CMakeLists.txt for verification specification library
cmake_minimum_required(VERSION 3.20)
project(verification C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译验证规约库为共享库
add_library(verification SHARED
    buffer.spec.c
    # 可以添加更多规约文件
    # memory.spec.c
    # integer.spec.c
    # float.spec.c
)

target_include_directories(verification PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置输出目录
set_target_properties(verification PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/specs
)

# 编译选项
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(verification PRIVATE
        -Wall -Wextra -Wpedantic
        -fPIC
    )
elseif(MSVC)
    target_compile_options(verification PRIVATE
        /W4
    )
endif()

# 安装规则
install(TARGETS verification
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES verification.h
    DESTINATION include
)
