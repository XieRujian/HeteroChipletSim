#include "ini.h"

namespace util
{
    bool IsSpace(char c) { return ' ' == c || '\t' == c; }

    void Trim(std::string &str)
    {
        if (str.empty())
            return;
        int start_pos = 0, end_pos = str.size();
        while (end_pos && IsSpace(str[end_pos - 1]))
            end_pos--;
        while (start_pos < end_pos && IsSpace(str[start_pos]))
            start_pos++;
        str = str.substr(start_pos, end_pos - start_pos);
    }

    bool AnalyseLine(const std::string &line, std::string &section, std::string &key, std::string &value)
    {
        if (line.empty())
            return false;
        int start_pos = 0, end_pos = line.size() - 1, pos, s_startpos, s_endpos;
        if ((pos = line.find("#")) != -1)
        {
            if (0 == pos)
                return false;
            end_pos = pos - 1;
        }
        if ((s_startpos = line.find("[")) != -1 && (s_endpos = line.find("]")) != -1)
        {
            section = line.substr(s_startpos + 1, s_endpos - 1);
            return true;
        }
        std::string new_line = line.substr(start_pos, start_pos + 1 - end_pos);
        if ((pos = new_line.find('=')) == -1)
            return false;
        key = new_line.substr(0, pos);
        value = new_line.substr(pos + 1, end_pos + 1 - (pos + 1));
        Trim(key);
        if (key.empty())
            return false;
        Trim(value);
        if ((pos = value.find("\r")) > 0)
            value.replace(pos, 1, "");
        if ((pos = value.find("\n")) > 0)
            value.replace(pos, 1, "");
        return true;
    }

    std::map<std::string, std::map<std::string, std::string>> ReadIniConfig(const std::string &filename)
    {
        std::map<std::string, std::map<std::string, std::string>> settings;
        std::ifstream infile(filename.c_str());
        if (!infile)
            return {};
        std::string line, key, value, section;
        std::map<std::string, std::string> &global_k_v = settings[""];
        std::map<std::string, std::string> *k_v = &global_k_v;

        while (getline(infile, line))
        {
            if (AnalyseLine(line, section, key, value))
            {
                if (settings.count(section))
                    (*k_v)[key] = value;
                else
                {
                    settings.emplace(section, global_k_v);
                    k_v = &settings.at(section);
                }
            }
            key.clear();
            value.clear();
        }
        infile.close();
        settings.erase("");
        return settings;
    }

    void WriteIniConfig(const std::string &filename, const std::map<std::string, std::map<std::string, std::string>> &config)
    {
        std::ofstream outfile(filename.c_str());
        if (!outfile)
            return;
        for (const auto &[section, k_v] : config)
        {
            if (!section.empty())
                outfile << "[" << section << "]\n";
            for (const auto &[key, value] : k_v)
                outfile << key << "=" << value << "\n";
            outfile << "\n";
        }
        outfile.close();
    }
}
