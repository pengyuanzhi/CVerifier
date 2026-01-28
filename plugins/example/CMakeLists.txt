# CMakeLists.txt for example plugin
cmake_minimum_required(VERSION 3.20)

# 插件名称
set(PLUGIN_NAME CustomChecker)

# 插件源文件
set(PLUGIN_SOURCES
    CustomChecker.cpp
)

# 创建共享库（插件）
add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})

# 设置输出名称
set_target_properties(${PLUGIN_NAME} PROPERTIES
    PREFIX ""  # 移除 lib 前缀（Windows 上也适用）
    OUTPUT_NAME ${PLUGIN_NAME}
)

# 查找 CVerifier 核心库
if(NOT CVERIFIER_ROOT)
    set(CVERIFIER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
endif()

# 包含目录
target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CVERIFIER_ROOT}/include
    ${CVERIFIER_ROOT}/build/include
)

# 链接库
target_link_libraries(${PLUGIN_NAME} PRIVATE
    cverifier-core
)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PLUGIN_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        -fPIC
        -fvisibility=hidden  # 隐藏符号，只导出必要的函数
    )
elseif(MSVC)
    target_compile_options(${PLUGIN_NAME} PRIVATE
        /W4
        /wd4251  # 禁用 dll interface 警告
    )
endif()

# 导出符号
if(NOT MSVC)
    target_link_options(${PLUGIN_NAME} PRIVATE
        -Wl,--as-needed
        -Wl,--no-undefined
    )
endif()

# 安装规则
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION lib/cverifier/plugins
    RUNTIME DESTINATION bin/cverifier/plugins
)

# 打印信息
message(STATUS "Configuring plugin: ${PLUGIN_NAME}")
message(STATUS "  Output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
