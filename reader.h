// KeyValueParser.h
#ifndef KEYVALUEPARSER_H
#define KEYVALUEPARSER_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

class KeyValueParser {
  public:
    KeyValueParser(const std::string &filename, char delimiter = ':')
        : m_filename(filename), m_delimiter(delimiter) {}

    bool parse() {
        std::ifstream file(m_filename.c_str());
        if (!file.is_open()) {
            std::cerr << "Unable to open file: " << m_filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key, value;

            if (!std::getline(iss, key, m_delimiter) ||
                !std::getline(iss, value)) {
                continue; // Skip malformed lines
            }
            trim(key);
            trim(value);
            m_data[key] = value;
        }

        file.close();
        return true;
    }

    const std::map<std::string, std::string> &data() const { return m_data; }

  private:
    std::string m_filename;
    char m_delimiter;
    std::map<std::string, std::string> m_data;

    void trim(std::string &s) {
        s.erase(s.begin(),
                std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
        s.erase(std::find_if(s.rbegin(), s.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                s.end());
    }
};

#endif // KEYVALUEPARSER_H
