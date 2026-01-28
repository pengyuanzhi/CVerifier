# CVerifier 需求规格说明书 - 补充文档

**本文档是 `requirements.md` 的补充，包含所有新增功能的详细需求。**

---

## 新增功能详细需求

### FR-011: 浮点溢出检测

**需求描述**：工具必须检测 IEEE 754 标准定义的所有浮点异常情况

**功能规格**：

#### 11.1 浮点上溢/下溢检测
- 检测结果超出类型表示范围的情况
- 上溢 → +Inf
- 下溢 → -Inf
- 支持以下操作：加、减、乘、除
- 支持类型：float, double, long double

**测试用例**：
```c
void test_float_overflow() {
    float a = 1.0e38f;
    float b = a * a;  // 应检测到：溢出 → Inf
}

void test_float_underflow() {
    float a = -1.0e38f;
    float b = a * a;  // 应检测到：溢出 → -Inf
}
```

#### 11.2 NaN 产生和传播检测
- 检测 NaN 的产生（0.0/0.0, sqrt(-1), Inf-Inf 等）
- 检测 NaN 的传播（任何涉及 NaN 的运算结果都是 NaN）
- 支持 Quiet NaN 和 Signaling NaN
- 记录 NaN 产生的位置和原因

**测试用例**：
```c
void test_nan_generation() {
    float a = 0.0f / 0.0f;    // 应检测到：产生 NaN
    float b = sqrt(-1.0f);   // 应检测到：产生 NaN
    float c = a + 1.0f;      // 应检测到：NaN 传播
}

void test_nan_propagation() {
    float a = NAN;
    float b = a * 2.0f;      // 应检测到：结果为 NaN
    float c = b + 3.0f;      // 应检测到：结果为 NaN
}
```

#### 11.3 Inf 产生和传播检测
- 检测 +Inf 的产生（1.0/0.0, 大数溢出）
- 检测 -Inf 的产生（-1.0/0.0, 负大数溢出）
- 检测 Inf 的传播规则

**测试用例**：
```c
void test_inf_generation() {
    float a = 1.0f / 0.0f;    // 应检测到：产生 +Inf
    float b = -1.0f / 0.0f;   // 应检测到：产生 -Inf
    float c = 1.0e38f * 2;     // 应检测到：溢出 → Inf
}
```

#### 11.4 精度损失检测
- 检测大数加小数导致的精度损失
- 检测类型转换精度损失（double → float）
- 检测累积舍入误差

**测试用例**：
```c
void test_precision_loss() {
    double large = 1.0e20;
    double small = 1.0;
    double result = large + small;  // 应检测到：精度损失
    // small 被"吞掉"
}

void test_conversion_precision() {
    double d = 1.23456789012345678;
    float f = (float)d;  // 应检测到：精度损失
    // 只保留约 7 位有效数字
}
```

#### 11.5 舍入误差检测
- 检测连续运算中的舍入误差累积
- 检测比较操作中的舍入误差

**测试用例**：
```c
void test_rounding_error() {
    float sum = 0.0f;
    for (int i = 0; i < 1000000; i++) {
        sum += 0.000001f;  // 应检测到：累积舍入误差
    }
    // 期望 1.0，但实际可能不是
}
```

**验收标准**：
- [ ] 能检测所有 IEEE 754 定义的异常情况
- [ ] 支持 float 和 double 类型
- [ ] 准确报告异常类型和位置
- [ ] 提供 NaN/Inf 的产生轨迹

---

### FR-012: 除零错误检测

**需求描述**：工具必须检测所有除以零的错误

**功能规格**：

#### 12.1 整数除零检测
- 检测整数除法（/）除数为 0
- 检测整数模运算（%）除数为 0

**测试用例**：
```c
void test_int_division_by_zero() {
    int a = 10;
    int b = 0;
    int c = a / b;  // 应检测到：整数除零
    int d = a % b;  // 应检测到：模运算除零
}
```

#### 12.2 浮点除零检测
- 检测浮点除法除数为 0.0
- 区分 0.0 和 -0.0（根据符号）
- 检测结果的正确性（+Inf 或 -Inf）

**测试用例**：
```c
void test_float_division_by_zero() {
    float a = 10.0f;
    float b = 0.0f;
    float c = a / b;  // 应检测到：浮点除零，产生 +Inf

    float d = -10.0f;
    float e = d / b;  // 应检测到：浮点除零，产生 -Inf
}
```

**验收标准**：
- [ ] 能检测所有类型的除零错误
- [ ] 准确报告除零位置
- [ ] 区分整数和浮点除零
- [ ] 提供反例（除数为 0 的值）

---

### FR-013: 规约系统

**需求描述**：工具必须支持 ACSL 风格的规约注解，实现代码与规约分离

**功能规格**：

#### 13.1 规约注解解析
- 支持 ACSL 风格的注解（/*@ ... */）
- 支持前置条件（requires）
- 支持后置条件（ensures）
- 支持断言（assert）
- 支持不变量（invariant）
- 支持分离逻辑（separated）
- 支持内存断言（\valid, \valid_read）
- 支持指针断言（\null, \initialized）

**规约注解示例**：
```c
/*@
  requires \valid_read(ptr);
  requires size > 0;
  requires \separated(ptr, oth);
  assigns \nothing;
  ensures \result == 0 || \result == 1;
*/
int process(char* ptr, int size, char* oth);

/*@
  requires size >= 0;
  ensures \result >= 0;
  ensures \result <= size;
*/
int safe_copy(char* dest, char* src, int size);

// 循环不变量
/*@
  loop invariant 0 <= i <= n;
  loop invariant \valid(arr + (0 .. n-1));
  loop assigns arr[0 .. i-1];
*/
for (int i = 0; i < n; i++) {
    arr[i] = 0;
}
```

#### 13.2 规约代码分离编译
- 规约代码可以独立编译为共享库（.so/.dll）
- 规约代码不包含在用户代码中
- 通过插件机制加载规约
- 支持规约版本管理

**目录结构**：
```
project/
├── src/                    # 用户业务代码
│   └── main.c
├── specs/                  # 规约代码
│   ├── buffer.spec.c
│   ├── memory.spec.c
│   └── libverification.so
└── cverifier              # 验证工具
```

#### 13.3 验证函数库
提供预定义的验证函数，包括：

**内存验证函数**：
```c
// 验证内存可读性
bool verifiable_read(void* ptr, int size);

// 验证内存可写性
bool verifiable_write(void* ptr, int size);

// 验证指针有效性
bool verifiable_pointer(void* ptr);

// 验证内存分离
bool verifiable_separated(void* ptr1, void* ptr2, int size);
```

**数值验证函数**：
```c
// 验证范围
bool in_range(int value, int min, int max);

// 验证无符号
bool is_positive(int value);

// 验证非零
bool is_nonzero(int value);
```

**断言函数**：
```c
// 运行时断言
void assert_true(bool condition, const char* msg);

// 静态断言（编译时）
#define STATIC_ASSERT(cond) _Static_assert(cond, #cond)
```

#### 13.4 规约隔离机制
- 规约代码使用特殊的命名空间或前缀
- 规约代码不使用用户代码的函数
- 规约代码的宏不影响用户代码
- 支持规约代码的沙箱执行

**隔离示例**：
```c
// 规约代码
#ifdef VERIFICATION_SPEC
int ver_buffer_check(char* ptr, int size) {
    // 验证逻辑
}
#endif

// 用户代码
#include "verification.h"

int main() {
    char buf[100];
    ver_buffer_check(buf, 100);  // 调用验证函数
    // ... 业务逻辑
}
```

**验收标准**：
- [ ] 支持 ACSL 风格的完整语法
- [ ] 规约代码可独立编译
- [ ] 提供至少 20 个预定义验证函数
- [ ] 支持插件式规约加载
- [ ] 规约不污染用户代码命名空间
- [ ] 验证失败时提供清晰的错误信息

---

### FR-014: 插件系统

**需求描述**：工具必须支持动态加载验证插件

**功能规格**：

#### 14.1 插件接口
- 定义统一的插件基类
- 支持插件注册和发现
- 支持插件依赖管理

**插件接口定义**：
```cpp
class VerificationPlugin {
public:
    virtual ~VerificationPlugin() = default;

    // 插件信息
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;

    // 加载插件
    virtual void initialize(const PluginConfig& config) = 0;

    // 验证函数
    virtual bool verify(LLIRFunction* func) = 0;

    // 报告函数
    virtual std::vector<VulnerabilityReport> getReports() const = 0;

    // 清理
    virtual void shutdown() = 0;
};
```

#### 14.2 插件加载器
- 支持从目录自动发现插件（.so/.dll）
- 支持配置文件指定插件
- 支持插件版本检查
- 支持插件依赖解析

**插件配置示例**：
```yaml
plugins:
  - name: BufferOverflowPlugin
    path: /usr/lib/cverifier/plugins/libbufferoverflow.so
    enabled: true
    priority: 10

  - name: CustomPlugin
    path: /home/user/.cverifier/plugins/libcustom.so
    enabled: true
    config: /home/user/.cverifier/custom.yaml
```

#### 14.3 插件沙箱
- 插件在独立进程中运行（可选）
- 插件崩溃不影响主程序
- 限制插件资源使用

**验收标准**：
- [ ] 支持动态加载 .so 和 .dll
- [ ] 插件能访问验证引擎接口
- [ ] 插件能添加新的检测器
- [ ] 提供至少 3 个内置插件示例
- [ ] 插件文档完整

---

### FR-015: Web UI 任务管理

**需求描述**：提供 Web 界面进行任务管理和结果查看

**功能规格**：

#### 15.1 任务管理
- 创建验证任务
  - 选择要验证的代码文件
  - 选择验证选项（检测器、超时、深度等）
  - 设置任务优先级
  - 批量任务创建

- 任务状态管理
  - 待处理（Pending）
  - 运行中（Running）
  - 已完成（Completed）
  - 失败（Failed）
  - 已取消（Cancelled）

- 任务操作
  - 取消任务
  - 重试失败任务
  - 删除任务
  - 暂停/恢复任务
  - 任务优先级调整

#### 15.2 进度监控
- 实时进度条
  - 显示当前分析进度百分比
  - 显示已用时间/预计剩余时间

- 资源监控
  - CPU 使用率
  - 内存使用量
  - 线程数

- 日志查看
  - 实时日志流
  - 日志级别过滤（ERROR, WARN, INFO, DEBUG）
  - 日志搜索

#### 15.3 代码编辑器
- 集成 Monaco Editor
- 语法高亮（C 语言）
- 代码折叠
- 代码导航
  - 跳转到定义
  - 查找引用
  - 文件大纲
- 实时标注
  - 漏洞位置高亮
  - 错误提示
  - 规约注解高亮

#### 15.4 结果查看
- 漏洞列表
  - 按类型筛选
  - 按严重程度筛选
  - 按文件筛选

- 错误详情
  - 漏洞描述
  - 漏洞位置（文件、行、列）
  - 错误轨迹
  - 反例展示
  - 修复建议

- 报告导出
  - PDF 格式
  - SARIF 格式
  - JSON 格式

#### 15.5 协作功能
- 多用户支持
- 权限管理
  - 管理员
  - 普通用户
  - 只读用户
- 审计日志
  - 操作日志
  - 登录日志

**技术规格**：

**前端**：
- React 18+
- TypeScript
- Ant Design / Material-UI
- Monaco Editor
- WebSocket 客户端
- Axios

**后端**：
- C++ Crow 框架
- WebSocket++ (实时通信)
- nlohmann/json (JSON处理)
- SQLite (数据库，可选)

**通信协议**：
- REST API
  - GET /api/tasks - 获取任务列表
  - POST /api/tasks - 创建任务
  - DELETE /api/tasks/{id} - 删除任务
  - GET /api/tasks/{id}/logs - 获取任务日志

- WebSocket
  - /ws/tasks/{id}/progress - 进度推送
  - /ws/tasks/{id}/logs - 日志推送

**验收标准**：
- [ ] Web UI 可用性评分 ≥ 80/100
- [ ] 支持同时管理 ≥ 10 个任务
- [ ] 响应时间 < 200ms（大多数操作）
- [ ] 实时更新延迟 < 500ms
- [ ] 代码编辑器支持 ≥ 10000 行文件
- [ ] 支持主流浏览器（Chrome, Firefox, Edge）

---

## 更新的非功能需求

### NFR-FLOAT-001: IEEE 754 合规性

**需求描述**：浮点检测必须完全符合 IEEE 754-2019 标准

**合规性要求**：
- 支持 binary32（单精度）
- 支持 binary64（双精度）
- 支持 extended precision（可选）
- 正确处理 NaN（Quiet/Signaling）
- 正确处理 Inf（+Inf/-Inf）
- 正确处理 Subnormal numbers
- 正确处理 Round-to-Nearest-Even

### NFR-SPEC-001: 规约代码隔离

**需求描述**：规约代码必须与业务代码完全隔离

**隔离要求**：
- 规约代码使用独立的命名空间
- 规约代码的宏不影响用户代码
- 规约代码可独立编译和链接
- 支持规约代码版本管理

### NFR-WEB-001: Web 性能

**需求描述**：Web UI 必须具有良好的性能

**性能指标**：
- 页面加载时间 < 3 秒
- 交互响应时间 < 200ms
- 支持并发用户数 ≥ 50
- 任务实时更新延迟 < 500ms

### NFR-PLUGIN-001: 插件安全性

**需求描述**：插件系统必须安全可靠

**安全要求**：
- 插件数字签名验证（可选）
- 插件沙箱执行
- 限制插件资源使用
- 插件崩溃不影响主程序

---

## 更新的验收标准

### 完整版验收标准（更新）

**功能验收**：
- [ ] 支持 6 类漏洞检测（增加浮点溢出、除零）
- [ ] 支持规约系统（ACSL 风格）
- [ ] 支持插件系统
- [ ] 提供 Web UI
- [ ] 准确率 ≥ 80%
- [ ] 误报率 ≤ 20%

**性能验收**：
- [ ] 分析速度 ≥ 1000 行代码/分钟
- [ ] 支持 10 个任务并发
- [ ] Web UI 响应时间 < 200ms

**文档验收**：
- [ ] 需求规格说明书（本）
- [ ] 架构设计文档（包含规约层、插件系统、Web架构）
- [ ] 详细设计文档（包含 C API、插件、浮点算法、Web API）
- [ ] 规约语言规范文档
- [ ] 插件 API 文档
- [ ] Web 架构文档
- [ ] C API 参考手册
- [ ] 用户手册（包含 Web UI 使用）
- [ ] 开发者指南

---

**文档结束**
