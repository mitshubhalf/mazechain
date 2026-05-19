#ifndef LOGGING_SYSTEM_H
#define LOGGING_SYSTEM_H

// MazeChain — Structured Logging System
// Levels: DEBUG, INFO, WARN, ERROR, CRITICAL
// Outputs: console + file (mazechain.log)
// Thread-safe, configurable via env or config file

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <map>

namespace Log {

    enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, CRITICAL = 4 };

    static Level       g_minLevel   = Level::INFO;
    static bool        g_toFile     = true;
    static bool        g_toConsole  = true;
    static std::string g_logPath    = "data/mazechain.log";
    static std::ofstream g_logFile;
    static std::mutex  g_logMutex;
    static size_t      g_maxSize    = 10 * 1024 * 1024; // 10MB rotation
    static std::map<std::string, Level> g_categories; // per-category override

    inline std::string levelStr(Level l) {
        switch (l) {
            case Level::DEBUG:    return "DEBUG";
            case Level::INFO:     return "INFO ";
            case Level::WARN:     return "WARN ";
            case Level::ERROR:    return "ERROR";
            case Level::CRITICAL: return "CRIT ";
        }
        return "?????";
    }

    inline std::string levelColor(Level l) {
        switch (l) {
            case Level::DEBUG:    return "\033[36m";  // cyan
            case Level::INFO:     return "\033[32m";  // green
            case Level::WARN:     return "\033[33m";  // yellow
            case Level::ERROR:    return "\033[31m";  // red
            case Level::CRITICAL: return "\033[35m";  // magenta
        }
        return "";
    }

    inline std::string timestamp() {
        auto now = std::time(nullptr);
        std::tm tm_info;
        localtime_r(&now, &tm_info);
        std::ostringstream ss;
        ss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    inline void init(const std::string& logPath = "data/mazechain.log",
                     Level minLevel = Level::INFO,
                     bool toFile = true, bool toConsole = true) {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logPath   = logPath;
        g_minLevel  = minLevel;
        g_toFile    = toFile;
        g_toConsole = toConsole;
        if (g_toFile) {
            g_logFile.open(logPath, std::ios::app);
        }
    }

    inline void rotate() {
        if (!g_logFile.is_open()) return;
        g_logFile.close();
        std::string backup = g_logPath + ".1";
        std::rename(g_logPath.c_str(), backup.c_str());
        g_logFile.open(g_logPath, std::ios::app);
    }

    inline void setLevel(Level l) { g_minLevel = l; }
    inline void setCategoryLevel(const std::string& cat, Level l) { g_categories[cat] = l; }

    inline Level parseLevel(const std::string& s) {
        if (s == "DEBUG") return Level::DEBUG;
        if (s == "WARN")  return Level::WARN;
        if (s == "ERROR") return Level::ERROR;
        if (s == "CRIT" || s == "CRITICAL") return Level::CRITICAL;
        return Level::INFO;
    }

    inline void write(Level level, const std::string& category, const std::string& msg) {
        std::lock_guard<std::mutex> lock(g_logMutex);

        Level effective = g_minLevel;
        auto it = g_categories.find(category);
        if (it != g_categories.end()) effective = it->second;
        if (level < effective) return;

        std::string ts   = timestamp();
        std::string line = "[" + ts + "] [" + levelStr(level) + "] [" + category + "] " + msg;

        if (g_toConsole) {
            std::cout << levelColor(level) << line << "\033[0m" << std::endl;
        }

        if (g_toFile && g_logFile.is_open()) {
            g_logFile << line << "\n";
            g_logFile.flush();
            // Check rotation
            auto pos = g_logFile.tellp();
            if (pos > 0 && (size_t)pos > g_maxSize) rotate();
        }
    }

    inline void debug(const std::string& cat, const std::string& msg)    { write(Level::DEBUG,    cat, msg); }
    inline void info(const std::string& cat, const std::string& msg)     { write(Level::INFO,     cat, msg); }
    inline void warn(const std::string& cat, const std::string& msg)     { write(Level::WARN,     cat, msg); }
    inline void error(const std::string& cat, const std::string& msg)    { write(Level::ERROR,    cat, msg); }
    inline void critical(const std::string& cat, const std::string& msg) { write(Level::CRITICAL, cat, msg); }

    // Convenience macros
    #define LOG_DEBUG(cat, msg)    Log::debug(cat, msg)
    #define LOG_INFO(cat, msg)     Log::info(cat, msg)
    #define LOG_WARN(cat, msg)     Log::warn(cat, msg)
    #define LOG_ERROR(cat, msg)    Log::error(cat, msg)
    #define LOG_CRITICAL(cat, msg) Log::critical(cat, msg)

} // namespace Log

#endif
