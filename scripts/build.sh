#!/bin/bash
# CVerifier 构建脚本

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "CVerifier Build Script"
echo "========================================="
echo ""

# 检查依赖
echo -e "${YELLOW}Checking dependencies...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake not found${NC}"
    echo "Please install CMake 3.20 or later"
    exit 1
fi

if ! command -v clang &> /dev/null; then
    echo -e "${YELLOW}Warning: Clang not found${NC}"
    echo "CVerifier requires Clang/LLVM 15+"
fi

if ! command -v z3 &> /dev/null; then
    echo -e "${YELLOW}Warning: Z3 not found${NC}"
    echo "CVerifier requires Z3 4.12+"
fi

echo -e "${GREEN}Dependencies check complete${NC}"
echo ""

# 设置构建类型
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-build}

echo "Build configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Directory: $BUILD_DIR"
echo ""

# 创建构建目录
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake 配置
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# 编译
echo -e "${YELLOW}Building CVerifier...${NC}"
cmake --build . -j$(nproc)

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "To install CVerifier, run:"
echo "  sudo cmake --install ."
echo ""
echo "To run tests, run:"
echo "  cd $BUILD_DIR && ctest"
echo ""
