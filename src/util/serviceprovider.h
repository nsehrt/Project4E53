#pragma once

#include "log.h"

class ServiceProvider
{
private:
    static std::shared_ptr<Logger<FileLogPolicy>> fileLogger;
    static std::shared_ptr<Logger<CLILogPolicy>> cliLogger;
    static std::shared_ptr<Logger<VSLogPolicy>> vsLogger;

public:
    static Logger<FileLogPolicy>* getFileLogger() { return fileLogger.get(); }
    static void setFileLoggingService(std::shared_ptr<Logger<FileLogPolicy>> providedFileLogger);

    static Logger<CLILogPolicy>* getCLILogger() { return cliLogger.get(); }
    static void setCLILoggingService(std::shared_ptr<Logger<CLILogPolicy>> providedFileLogger);

    static Logger<VSLogPolicy>* getVSLogger() { return vsLogger.get(); }
    static void setVSLoggingService(std::shared_ptr<Logger<VSLogPolicy>> providedFileLogger);
};