#include <ShlObj.h>
#include <Shlwapi.h>
#include <PathCch.h>

#include "log.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Pathcch.lib")

//bool FileLogPolicy::openOutputStream(const std::wstring& filename)
//{
//    PWSTR docPath = NULL;
//    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, NULL, NULL, &docPath);
//
//    if (FAILED(hr))
//        return false;
//
//    std::wstringstream path;
//    path << docPath << L"\\4E53\\logs\\";
//
//    ::CoTaskMemFree(static_cast<void*>(docPath));
//
//    hr = SHCreateDirectory(NULL, path.str().c_str());
//
//    if (FAILED(hr))
//        return false;
//
//    path << filename.c_str();
//
//    outputStream.open(path.str().c_str(), std::ios_base::binary | std::ios_base::out);
//
//    if (!outputStream.is_open())
//        return false;
//
//    outputStream.precision(20);
//
//    return true;
//}
//
//void FileLogPolicy::closeOutputStream()
//{
//    outputStream.close();
//}
//
//void FileLogPolicy::write(const std::string& msg)
//{
//    outputStream << msg << std::endl;
//}
//
//bool CLILogPolicy::openOutputStream(const std::wstring& /*name*/)
//{
//    return true;
//}
//
//void CLILogPolicy::closeOutputStream()
//{
//}
//
//void CLILogPolicy::write(const std::string& msg)
//{
//    std::cout << msg << std::endl;
//}

bool LogPolicy::openOutputStream(const std::wstring& name)
{
    return true;
}

void LogPolicy::closeOutputStream()
{
}

void LogPolicy::write(const std::string& msg)
{
    OutputDebugStringA(msg.c_str());
}
