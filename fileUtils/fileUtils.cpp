#include "pch.h"
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <filesystem>
#include "framework.h"
#include "fileUtils.h"

bool FileUtils::findTheFileOrFolder(const std::string &sName, std::filesystem::path& _path)
{
    std::wstring buffer;
    buffer.resize(1024);
    GetModuleFileNameW(nullptr, &buffer[0], (DWORD)buffer.size());

    std::filesystem::path path = buffer;

    path.remove_filename();

    // go up the tree and find the folder
    for ( ; ; )
    {
        std::filesystem::path tmp = path;
        tmp.append(sName);
        if (std::filesystem::exists(tmp))
        {
            _path = tmp;
            return true;
        }
        tmp = path.parent_path();
        if (tmp == path)
        {
            return false;
        }
        path = tmp;
    }
}

