#pragma once

#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <fstream>

enum LogLevel
{
    DEBUG, INFO, WARNING, ERROR, CRITICAL,
};

struct Logger
{
    LogLevel _loglevel;
    std::chrono::system_clock::time_point _logtime;
    std::thread::id _thread_id;
    std::string _logmessage;
    std::string _filename;
};

class Log
{
public:
    static Log& Singleton()
    {
        static Log singleton;
        return singleton;
    }

    Log()
    {
        __running = true;
    }
    Log(const Log& other) = delete;
    Log& operator=(const Log&) = delete;

    Log(const std::string filename)
        : __logfile(filename)
    {
        __logthread = std::thread(&Log::LogWork, this);
        __running = true;
    }

    ~Log()
    {
        __running = false;
        __cv.notify_one();
        if (__logthread.joinable())
        {
            __logthread.join();
        }
        __logfile.close();
    }

    void LogWrite(const Logger& logger)
    {
        std::lock_guard<std::mutex> lock(__mtx);
        __queue.emplace(logger);
        __cv.notify_one();
    }

private:
    bool __running;
    std::queue<Logger> __queue;
    std::thread __logthread;
    std::mutex __mtx;
    std::condition_variable __cv;
    std::ofstream __logfile;

    void LogWork()
    {
        while (__running)
        {
            std::unique_lock<std::mutex> lock(__mtx);
            __cv.wait(lock, [&]()
                {
                    return !__queue.empty() || !__running;
                });
            while (!__queue.empty())
            {
                auto logger = std::move(__queue.front());
                __queue.pop();
                lock.unlock();

                std::ostringstream oss;
                oss << " now time : " << std::chrono::system_clock::to_time_t(logger._logtime) << " ";
                oss << "( thread id : " << logger._thread_id << " ) ";
                oss << "[" << LogLevelToString(logger._loglevel) << "] ";
                oss << logger._filename << ": " << logger._logmessage << std::endl;
                __logfile << oss.str();
                __logfile.flush();

                std::cout << oss.str() << std::endl;

                lock.lock();
            }
        }
    }

    std::string LogLevelToString(const LogLevel& level)
    {
        switch (level)
        {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
        }
    }
};

class LoggerFunc
{
public:
    Log log;

    LoggerFunc(const std::string& filename)
        : log(filename) {}

    void Debug(const std::string& message)
    {
        LogMessage(LogLevel::DEBUG, message);
    }

    void Info(const std::string& message)
    {
        LogMessage(LogLevel::INFO, message);
    }

    void Warning(const std::string& message)
    {
        LogMessage(LogLevel::WARNING, message);
    }

    void Error(const std::string& message)
    {
        LogMessage(LogLevel::ERROR, message);
    }

    void Critical(const std::string& message)
    {
        LogMessage(LogLevel::CRITICAL, message);
    }

private:
    void LogMessage(LogLevel loglevel, const std::string& message)
    {
        auto nowtime = std::chrono::system_clock::now();
        auto thread_id = std::this_thread::get_id();

        Logger logger{ loglevel, nowtime, thread_id, message };
        log.LogWrite(logger);
    }
};