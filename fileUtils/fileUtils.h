#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct FileUtils
{
    static bool findTheFileOrFolder(const std::string &sName, std::filesystem::path &path);
};
