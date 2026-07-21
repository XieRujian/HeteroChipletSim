#include <fstream>
#include <stdexcept>
#include <sys/stat.h>

#include "common.h"

namespace util
{
    std::string GetFileContent(const std::string &path)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file)
        {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string content(size, '\0');
        if (!file.read(&content[0], size))
            throw std::runtime_error("Failed to read file content: " + path);

        return content;
    }

    void MakeDirectory(const std::string &path)
    {
        mkdir(path.c_str(), 0755);
    }

    double pow(double x, int n)
    {
        double result = 1.0;
        while (n)
        {
            if (n & 1)
                result *= x;
            x *= x;
            n >>= 1;
        }
        return result;
    }
}