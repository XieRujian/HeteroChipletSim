#ifndef INI_H
#define INI_H
#include <string>
#include <map>
#include <fstream>

namespace util
{
    std::map<std::string, std::map<std::string, std::string>> ReadIniConfig(const std::string &filename);
    void WriteIniConfig(const std::string &filename, const std::map<std::string, std::map<std::string, std::string>> &config);
}

#endif // INI_H