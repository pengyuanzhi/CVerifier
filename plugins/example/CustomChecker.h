/**
 * CustomChecker.h - 示例插件：自定义检查器
 *
 * 演示如何创建 CVerifier 插件
 */

#ifndef CUSTOM_CHECKER_H
#define CUSTOM_CHECKER_H

#include "cverifier/IPlugin.h"
#include "cverifier/LLIRModule.h"
#include <string>
#include <vector>

namespace cverifier {

/**
 * 自定义检查器插件示例
 *
 * 功能：
 * - 检测未初始化的变量使用
 * - 检测潜在的死代码
 * - 检测循环中的浮点累积误差
 */
class CustomChecker : public IPlugin {
public:
    CustomChecker();
    ~CustomChecker() override;

    // IPlugin 接口实现
    std::string getName() const override;
    std::string getVersion() const override;
    std::string getDescription() const override;

    void initialize(const PluginConfig& config) override;
    bool verify(LLIRFunction* function) override;
    std::vector<VulnerabilityReport> getReports() const override;
    void shutdown() override;

    // 健康检查
    bool isHealthy() const override;

private:
    /**
     * 检测未初始化变量
     */
    void checkUninitializedVariables(LLIRFunction* function);

    /**
     * 检测死代码
     */
    void checkDeadCode(LLIRFunction* function);

    /**
     * 检测浮点累积误差
     */
    void checkFloatAccumulation(LLIRFunction* function);

    /**
     * 创建漏洞报告
     */
    VulnerabilityReport createReport(
        const SourceLocation& loc,
        const std::string& type,
        const std::string& message,
        Severity severity
    );

private:
    PluginConfig config_;
    std::vector<VulnerabilityReport> reports_;
    bool initialized_;
};

} // namespace cverifier

#endif // CUSTOM_CHECKER_H
