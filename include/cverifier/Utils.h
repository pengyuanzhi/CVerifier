#ifndef CVERIFIER_UTILS_H
#define CVERIFIER_UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <random>
#include <iostream>
#include <chrono>

namespace cverifier {
namespace utils {

/**
 * @brief 字符串工具类
 */
class StringUtils {
public:
    /**
     * @brief 分割字符串
     * @param str 输入字符串
     * @param delimiter 分隔符
     * @return 分割后的字符串列表
     */
    static std::vector<std::string> split(
        const std::string& str,
        char delimiter
    ) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    /**
     * @brief 连接字符串
     * @param strings 字符串列表
     * @param delimiter 分隔符
     * @return 连接后的字符串
     */
    static std::string join(
        const std::vector<std::string>& strings,
        const std::string& delimiter
    ) {
        if (strings.empty()) {
            return "";
        }

        std::ostringstream oss;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i > 0) {
                oss << delimiter;
            }
            oss << strings[i];
        }

        return oss.str();
    }

    /**
     * @brief 去除字符串首尾空格
     */
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return "";
        }

        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    /**
     * @brief 将字符串转为小写
     */
    static std::string toLower(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = std::tolower(c);
        }
        return result;
    }

    /**
     * @brief 判断字符串是否以指定前缀开始
     */
    static bool startsWith(
        const std::string& str,
        const std::string& prefix
    ) {
        if (prefix.size() > str.size()) {
            return false;
        }
        return str.substr(0, prefix.size()) == prefix;
    }

    /**
     * @brief 判断字符串是否以指定后缀结束
     */
    static bool endsWith(
        const std::string& str,
        const std::string& suffix
    ) {
        if (suffix.size() > str.size()) {
            return false;
        }
        return str.substr(str.size() - suffix.size()) == suffix;
    }
};

/**
 * @brief 文件路径工具类
 */
class PathUtils {
public:
    /**
     * @brief 获取文件名（不含路径）
     */
    static std::string getFileName(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos) {
            return path;
        }
        return path.substr(pos + 1);
    }

    /**
     * @brief 获取文件扩展名
     */
    static std::string getExtension(const std::string& path) {
        std::string filename = getFileName(path);
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos) {
            return "";
        }
        return filename.substr(pos);
    }

    /**
     * @brief 获取目录路径（不含文件名）
     */
    static std::string getDirectory(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos) {
            return ".";
        }
        return path.substr(0, pos);
    }

    /**
     * @brief 连接路径
     */
    static std::string join(
        const std::string& path1,
        const std::string& path2
    ) {
        std::string result = path1;
        if (!result.empty() &&
            result.back() != '/' &&
            result.back() != '\\') {
            result += '/';
        }
        result += path2;
        return result;
    }
};

/**
 * @brief 格式化工具类
 */
class FormatUtils {
public:
    /**
     * @brief 格式化数字（带千分位分隔符）
     */
    static std::string formatNumber(int64_t num) {
        std::ostringstream oss;
        oss << std::fixed << num;
        return oss.str();
    }

    /**
     * @brief 格式化字节数
     */
    static std::string formatBytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return oss.str();
    }

    /**
     * @brief 格式化时间（秒 -> 可读格式）
     */
    static std::string formatTime(double seconds) {
        int hours = static_cast<int>(seconds) / 3600;
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        int secs = static_cast<int>(seconds) % 60;

        std::ostringstream oss;
        if (hours > 0) {
            oss << hours << "h ";
        }
        if (minutes > 0 || hours > 0) {
            oss << minutes << "m ";
        }
        oss << secs << "s";

        return oss.str();
    }
};

/**
 * @brief 日志工具类
 */
class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    static void setLevel(Level level) {
        instance().minLevel_ = level;
    }

    static void debug(const std::string& message) {
        instance().log(Level::Debug, message);
    }

    static void info(const std::string& message) {
        instance().log(Level::Info, message);
    }

    static void warning(const std::string& message) {
        instance().log(Level::Warning, message);
    }

    static void error(const std::string& message) {
        instance().log(Level::Error, message);
    }

private:
    Level minLevel_ = Level::Info;

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void log(Level level, const std::string& message) {
        if (level < minLevel_) {
            return;
        }

        const char* levelStr;
        switch (level) {
            case Level::Debug:   levelStr = "DEBUG"; break;
            case Level::Info:    levelStr = "INFO "; break;
            case Level::Warning: levelStr = "WARN "; break;
            case Level::Error:   levelStr = "ERROR"; break;
            default:             levelStr = "UNKNOWN"; break;
        }

        std::cerr << "[" << levelStr << "] " << message << std::endl;
    }
};

/**
 * @brief 计时器类
 */
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    /**
     * @brief 重置计时器
     */
    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief 获取经过的时间（毫秒）
     */
    double elapsedMs() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    /**
     * @brief 获取经过的时间（秒）
     */
    double elapsedSec() const {
        return elapsedMs() / 1000.0;
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

/**
 * @brief 随机数生成器类
 */
class Random {
public:
    static int nextInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(instance().rng_);
    }

    static double nextDouble(double min, double max) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(instance().rng_);
    }

    static std::string nextString(size_t length) {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += alphanum[nextInt(0, sizeof(alphanum) - 2)];
        }
        return result;
    }

private:
    static Random& instance() {
        static Random random;
        return random;
    }

    std::mt19937 rng_;
    std::random_device rd_;

    Random() : rng_(rd_()) {}
};

} // namespace utils
} // namespace cverifier

#endif // CVERIFIER_UTILS_H
