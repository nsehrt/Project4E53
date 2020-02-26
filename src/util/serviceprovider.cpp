#include "serviceprovider.h"

std::shared_ptr<Logger<FileLogPolicy>>ServiceProvider::fileLogger = NULL;
std::shared_ptr<Logger<CLILogPolicy>>ServiceProvider::cliLogger = NULL;
std::shared_ptr<Logger<VSLogPolicy>>ServiceProvider::vsLogger = NULL;

void ServiceProvider::setFileLoggingService(std::shared_ptr<Logger<FileLogPolicy>> providedFileLogger)
{
    fileLogger = providedFileLogger;
}


void ServiceProvider::setCLILoggingService(std::shared_ptr<Logger<CLILogPolicy>> providedFileLogger)
{
    cliLogger = providedFileLogger;
}

void ServiceProvider::setVSLoggingService(std::shared_ptr<Logger<VSLogPolicy>> providedFileLogger)
{
    vsLogger = providedFileLogger;
}