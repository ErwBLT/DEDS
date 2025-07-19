#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <unordered_map>
#include "../exchanges/Pool.h"

struct Pool;

// Utility functions for file operations and data formatting
struct Utils {
    static std::string loadFile(const std::string &path);

    static std::unordered_map<std::string, std::unique_ptr<Pool> > initPools(const std::string &path);
};

#endif //UTILS_H
