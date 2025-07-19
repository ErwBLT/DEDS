#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Utils.h"
#include <iomanip>

// Load file content as string
std::string Utils::loadFile(const std::string &path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (const std::exception &e) {
        std::cerr << "Error loading: " << e.what() << std::endl;
        return "";
    }
}

// Initialize pools from address list file
std::unordered_map<std::string, std::unique_ptr<Pool> > Utils::initPools(const std::string &path) {
    std::unordered_map<std::string, std::unique_ptr<Pool> > pools;

    try {
        std::string content = loadFile(path);
        std::istringstream stream(content);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (!line.empty()) {
                auto poolPtr = std::make_unique<Pool>();
                poolPtr->address = line;
                pools[line] = std::move(poolPtr);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error reading pool file: " << e.what() << std::endl;
    }

    return pools;
}
