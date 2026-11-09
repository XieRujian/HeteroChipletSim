#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "common.h"

namespace util
{
    std::vector<std::string> GetNextAvailableFileNames(const std::vector<std::string> &paths)
    {
        namespace fs = std::filesystem;
        for (std::size_t index = 0;; ++index)
        {
            std::vector<std::string> result;
            bool available = true;
            for (const auto &path : paths)
            {
                fs::path p(path);
                fs::path candidate = p.parent_path() /
                                     (p.stem().string() + "_" + std::to_string(index) + p.extension().string());
                if (fs::exists(candidate))
                {
                    available = false;
                    break;
                }
                result.push_back(candidate.string());
            }
            if (available)
                return result;
        }
    }

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
        namespace fs = std::filesystem;
        fs::path dir(path);
        fs::create_directory(dir);
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