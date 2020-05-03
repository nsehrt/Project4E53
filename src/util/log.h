#pragma once

#include <Windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <iomanip>

/*USAGE:
 LOG(Severity::Info, "Var a = " << a);
*/
#define LOG(sev, s )           \
{                            \
   std::stringstream os_;    \
   os_ << s;                   \
   ServiceProvider::getLogger()->print<sev>(os_.str().c_str());  \
}

enum Severity
{
    Info = 0,
    Debug,
    Warning,
    Critical,
    Error
};

/*abstract class*/
class LogPolicyAbstract
{
public:
    virtual ~LogPolicyAbstract() noexcept = default;

    virtual bool openOutputStream(const std::wstring& name) = 0;
    virtual void closeOutputStream() = 0;
    virtual void write(const std::string& msg) = 0;
};

/*implements vs output logger*/
class LogPolicy : public LogPolicyAbstract
{
private:
#ifndef _DEBUG
    std::ofstream outputStream;
#endif
public:
    LogPolicy() {};
    ~LogPolicy() {};

    bool openOutputStream(const std::wstring& name) override;
    void closeOutputStream() override;
    void write(const std::string& msg) override;
};

template<typename LogPolicy> class Logger;

template<typename LogPolicy>
void loggingDaemon(Logger<LogPolicy>* logger)
{
    std::unique_lock<std::timed_mutex> lock(logger->writeMutex, std::defer_lock);
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });
        if (logger->logBuffer.size())
        {
            if (!lock.try_lock_for(std::chrono::milliseconds{ 50 }))
                continue;
            for (auto& x : logger->logBuffer)
                logger->policy.write(x);
            logger->logBuffer.clear();
            lock.unlock();
        }
    } while (logger->isStillRunning.test_and_set() || logger->logBuffer.size());
}

template<typename LogPolicy>
class Logger
{
private:
    unsigned int logLineNumber;
    std::map<std::thread::id, std::string> threadName;
    LogPolicy policy;
    std::timed_mutex writeMutex;
    std::vector<std::string> logBuffer;
    std::thread daemon;
    std::atomic_flag isStillRunning{ ATOMIC_FLAG_INIT };

public:
    Logger(const std::wstring& name);
    ~Logger();

    void setThreadName(const std::string& name);
    template<Severity severity>
    void print(std::stringstream stream);
    template<Severity severity>
    void print(std::string msg);

    template<typename Policy>
    friend void loggingDaemon(Logger<Policy>* logger);
};

template<typename LogPolicy>
Logger<LogPolicy>::Logger(const std::wstring& name) : logLineNumber(0), threadName(), policy(), writeMutex(), logBuffer()
{
    if (policy.openOutputStream(name))
    {
        isStillRunning.test_and_set();
        daemon = std::move(std::thread{ loggingDaemon<LogPolicy>, this });
    }
    else
    {
        throw std::runtime_error("Unable to open logger!");
    }
}

template<typename LogPolicy>
Logger<LogPolicy>::~Logger()
{
    print<Severity::Info>("Logger is shutting down.");
    isStillRunning.clear();
    daemon.join();

    threadName.clear();
    std::map<std::thread::id, std::string>().swap(threadName);

    logBuffer.clear();
    logBuffer.shrink_to_fit();
    policy.closeOutputStream();
}

template<typename LogPolicy>
void Logger<LogPolicy>::setThreadName(const std::string& name)
{
    threadName[std::this_thread::get_id()] = name;
}

template<typename LogPolicy>
template<Severity severity>
void Logger<LogPolicy>::print(std::stringstream stream)
{
    std::stringstream logStream;

    SYSTEMTIME localTime;
    GetLocalTime(&localTime);

    if (logLineNumber != 0)
        logStream << "\r\n";
    logStream << std::setfill('0') << std::setw(3) << logLineNumber++ << ": " << localTime.wDay << "/" << localTime.wMonth << "/" << localTime.wYear <<
        " " << std::setw(2) << localTime.wHour << ":" << std::setw(2) << localTime.wMinute << ":" << std::setw(2) << localTime.wSecond << "\t";

    switch (severity)
    {
        case Severity::Info:
            logStream << "INFO:    ";
            break;
        case Severity::Debug:
            logStream << "DBUG:    ";
            break;
        case Severity::Warning:
            logStream << "WARN:    ";
            break;
        case Severity::Critical:
            logStream << "CRIT:    ";
            break;
        case Severity::Error:
            logStream << "ERROR:   ";
            break;
    };

    logStream << threadName[std::this_thread::get_id()] << ":\t";
    logStream << stream.str();
    std::lock_guard<std::timed_mutex> lock(writeMutex);
    logBuffer.push_back(logStream.str());
}

template<typename LogPolicy>
template<Severity severity>
void Logger<LogPolicy>::print(std::string msg)
{
    std::stringstream stream;
    stream << msg.c_str();
    this->print<severity>(std::stringstream(stream.str()));
}