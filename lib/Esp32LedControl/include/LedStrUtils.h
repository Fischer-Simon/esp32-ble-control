#pragma once

#include <sstream>
#include <string>

// trim from start (in place)
inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (in place)
inline void ltrim(std::string& s, char c) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [c](unsigned char ch) {
        return ch != c;
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s, char c) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [c](unsigned char ch) {
        return ch != c;
    }).base(), s.end());
}

inline void trim(std::string& s, char c) {
    ltrim(s, c);
    rtrim(s, c);
}

inline std::string trim_copy(std::string s) {
    ltrim(s);
    rtrim(s);
    return s;
}

inline std::vector<std::string> split_str(const std::string& string, char delimiter) {
    std::stringstream stream{string};
    std::vector<std::string> strings;
    std::string str;
    while (std::getline(stream, str, delimiter)) {
        strings.push_back(str);
    }
    return strings;
}
