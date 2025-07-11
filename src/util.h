#pragma once

#include <string>
#include <algorithm>

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return !std::isspace(ch);
            }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
}

static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

bool ReadEnvFlag(const char *name, bool def = false);
int ReadEnvInt(const char *name, int def = 0);
