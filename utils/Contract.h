#ifndef CONTRACT_H
#define CONTRACT_H

#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Smart contract interface for encoding/decoding function calls
class Contract {
public:
    explicit Contract(std::string _address, const std::string &abiPath);

    // Decode blockchain response data
    json decodeResponse(const std::string &responseData, const std::string &functionName);

    static std::string decodeAddress(const std::string &paddedAddress);

    static std::string decodeUint(const std::string &hexValue);

    static std::string decodeInt(const std::string &hexValue);

    static std::string decodeBool(const std::string &hexValue);

    static std::string decodeBytes(const std::string &data, size_t offset);

    static std::string decodeString(const std::string &data, size_t offset);


    // Encode function calls for blockchain transactions
    std::string encodeFunction(const std::string &name, const json &params = json::array());

    static std::string encodeParameters(const std::vector<std::string> &types, const std::vector<std::string> &values);

    static std::string encodeAddress(const std::string &value);

    static std::string encodeBool(bool value);

    static std::string encodeString(const std::string &value);

    static std::string encodeBytes(const std::string &value);

    static std::string encodeInt(const std::string &value);

    static std::string encodeUint(const std::string &value);

    std::string address{};
    nlohmann::json abi{};

private:
    std::map<std::string, nlohmann::json> functionsByName{};
};

#endif //CONTRACT_H
