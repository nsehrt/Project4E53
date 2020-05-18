#include <ShlObj.h>
#include <Shlwapi.h>
#include <PathCch.h>

#include "log.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Pathcch.lib")

bool LogPolicy::openOutputStream(const std::wstring& name)
{

    TCHAR NPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, NPath);

    std::wstringstream path;
    path << NPath << L"\\logs";

    HRESULT hr = SHCreateDirectory(NULL, path.str().c_str());

    if (FAILED(hr))
        return false;

    path << L"\\game.log";

    outputStream.open(path.str().c_str(), std::ios_base::binary | std::ios_base::out);

    if (!outputStream.is_open())
        return false;

    outputStream.precision(20);

    return true;
}

void LogPolicy::closeOutputStream()
{
    if (outputStream.is_open())
        outputStream.close();
}

void LogPolicy::write(const std::string& msg)
{
    /*output to cmd and file*/
    OutputDebugStringA(msg.c_str());

    outputStream << msg << std::flush;

}