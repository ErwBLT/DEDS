#include "Web3Client.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <utility>
#include <cryptopp/eccrypto.h>
#include "Contract.h"
#include <iostream>
#include "Utils.h"

using json = nlohmann::json;

// Constructor: Load ABI and index functions by name
Contract::Contract(std::string _address, const std::string &abiPath)
    : address{std::move(_address)} {
    abi = json::parse(Utils::loadFile(abiPath));
    for (const auto &func: abi) {
        if (func.contains("type") && func["type"] == "function" && func.contains("name")) {
            std::string name{func["name"]};
            functionsByName[name] = func;
        }
    }
}

// Encode function call with parameters for blockchain transaction
std::string Contract::encodeFunction(const std::string &name, const json &params) {
    if (!functionsByName.contains(name)) {
        throw std::runtime_error{"Function not found in ABI: " + name};
    }

    const nlohmann::json &func = functionsByName[name];

    std::string signature = name + "(";

    std::vector<std::string> types;
    if (func.contains("inputs") && func["inputs"].is_array()) {
        bool first = true;
        for (const auto &input: func["inputs"]) {
            if (!first) signature += ",";
            if (input.contains("type")) {
                const std::string &type = input["type"].get<std::string>();
                signature += type;
                types.push_back(type);
                first = false;
            }
        }
    }
    signature += ")";

    if (types.size() != params.size()) {
        throw std::runtime_error("Parameter count mismatch: expected " +
                                 std::to_string(types.size()) + ", got " +
                                 std::to_string(params.size()));
    }

    const std::string signatureHash = Web3Client::keccak256(signature);
    const std::string selector = Web3Client::bytesToHex(signatureHash.substr(0, 4)).substr(2, 8);

    std::string parametersData;
    if (!types.empty()) {
        std::vector<std::string> paramStrings;
        for (const auto &p: params) {
            paramStrings.push_back(p.dump());
        }
        parametersData = encodeParameters(types, paramStrings);
    }

    return "0x" + selector + parametersData;
}

// Encode multiple parameters according to ABI specification
std::string Contract::encodeParameters(const std::vector<std::string> &types, const std::vector<std::string> &values) {
    if (types.size() != values.size()) {
        throw std::runtime_error("Parameter count mismatch");
    }

    std::string headBlock;
    std::string tailBlock;
    size_t dynamicOffset = types.size() * 32;

    for (size_t i = 0; i < types.size(); i++) {
        const std::string &type = types[i];
        const std::string &value = values[i];

        if (!type.find('[')) {
            throw std::runtime_error("Array not supported");
        }

        if (type == "address") {
            headBlock += encodeAddress(value);
        } else if (type == "bool") {
            bool boolValue = (value == "true" || value == "1" || value == "0x1");
            headBlock += encodeBool(boolValue);
        } else if (type.find("uint") == 0) {
            headBlock += encodeUint(value);
        } else if (type.find("int") == 0) {
            headBlock += encodeInt(value);
        } else if (type.find("bytes") == 0 && type.length() > 5) {
            std::string cleanHex = value.substr(0, 2) == "0x" ? value.substr(2) : value;

            const int bytesSize = std::stoi(type.substr(5));
            if (cleanHex.length() > bytesSize * 2) {
                throw std::runtime_error("Bytes value too long for type " + type);
            }

            cleanHex.resize(64, '0');
            headBlock += cleanHex;
        } else if (type == "string") {
            std::stringstream ss;
            ss << std::hex << std::setfill('0') << std::setw(64) << dynamicOffset;
            headBlock += ss.str();

            const std::string encodedString = encodeString(value);
            tailBlock += encodedString;
            dynamicOffset += encodedString.length() / 2;
        } else if (type == "bytes") {
            std::stringstream ss;
            ss << std::hex << std::setfill('0') << std::setw(64) << dynamicOffset;
            headBlock += ss.str();

            const std::string encodedBytes = encodeBytes(value);
            tailBlock += encodedBytes;
            dynamicOffset += encodedBytes.length() / 2;
        } else {
            throw std::runtime_error("Unsupported parameter type: " + type);
        }
    }

    return headBlock + tailBlock;
}

// Encode Ethereum address to 32-byte hex string
std::string Contract::encodeAddress(const std::string &value) {
    std::string cleanAddress = value;

    if (cleanAddress.length() >= 2 && cleanAddress.substr(0, 2) == "0x") {
        cleanAddress = cleanAddress.substr(2);
    }

    if (cleanAddress.length() != 40) {
        throw std::runtime_error("Invalid Ethereum address format: " + value);
    }

    if (cleanAddress.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        throw std::runtime_error("Invalid hex characters in address: " + value);
    }

    return std::string(24, '0') + cleanAddress;
}

// Encode boolean to 32-byte hex string
std::string Contract::encodeBool(const bool value) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(64) << (value ? 1 : 0);
    return ss.str();
}

// Encode string with length prefix and padding
std::string Contract::encodeString(const std::string &value) {
    const size_t length = value.length();
    std::stringstream lengthHex;
    lengthHex << std::hex << std::setfill('0') << std::setw(64) << length;

    std::stringstream dataHex;
    for (char c: value) {
        dataHex << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
    }

    std::string data = dataHex.str();
    const size_t padLength = (32 - (length % 32)) % 32;
    data.append(padLength * 2, '0');

    return lengthHex.str() + data;
}

// Encode bytes with length prefix and padding
std::string Contract::encodeBytes(const std::string &value) {
    std::string cleanValue = value;
    if (cleanValue.length() >= 2 && cleanValue.substr(0, 2) == "0x") {
        cleanValue = cleanValue.substr(2);
    }

    const size_t length = cleanValue.length() / 2;
    std::stringstream lengthHex;
    lengthHex << std::hex << std::setfill('0') << std::setw(64) << length;

    const size_t padLength = (32 - (length % 32)) % 32;
    cleanValue.append(padLength * 2, '0');

    return lengthHex.str() + cleanValue;
}

// Encode signed integer to 32-byte hex string
std::string Contract::encodeInt(const std::string &value) {
    try {
        long long intValue = std::stoll(value);
        std::stringstream ss;

        if (intValue >= 0) {
            ss << std::hex << std::setfill('0') << std::setw(64) << intValue;
        } else {
            uint64_t unsignedValue = intValue;
            ss << std::hex << std::setfill('f') << std::setw(64) << unsignedValue;
        }

        return ss.str();
    } catch (const std::exception &e) {
        throw std::runtime_error("Invalid integer value: " + value);
    }
}

// Encode unsigned integer to 32-byte hex string
std::string Contract::encodeUint(const std::string &value) {
    try {
        unsigned long long uintValue = std::stoull(value);
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(64) << uintValue;
        return ss.str();
    } catch (const std::exception &e) {
        throw std::runtime_error("Invalid unsigned integer value: " + value);
    }
}

// Decode function response data according to ABI outputs
json Contract::decodeResponse(const std::string &responseData, const std::string &functionName) {
    if (!functionsByName.contains(functionName)) {
        throw std::runtime_error{"Function not found in ABI: " + functionName};
    }

    const nlohmann::json &func = functionsByName[functionName];

    if (!func.contains("outputs") || !func["outputs"].is_array()) {
        return json::object();
    }

    json result;
    std::string cleanData = responseData;
    if (cleanData.length() >= 2 && cleanData.substr(0, 2) == "0x") {
        cleanData = cleanData.substr(2);
    }

    size_t offset = 0;
    for (const auto &output: func["outputs"]) {
        if (!output.contains("type")) continue;
        std::string type = output["type"].get<std::string>();
        std::string name = output.contains("name") ? output["name"].get<std::string>() : "";


        if (type == "address") {
            std::string value = decodeAddress(cleanData.substr(offset, 64));
            result[name] = value;
            offset += 64;
        } else if (type.find("uint") == 0) {
            std::string value = decodeUint(cleanData.substr(offset, 64));
            result[name] = value;
            offset += 64;
        } else if (type.find("int") == 0) {
            std::string value = decodeInt(cleanData.substr(offset, 64));
            result[name] = value;
            offset += 64;
        } else if (type == "bool") {
            std::string value = decodeBool(cleanData.substr(offset, 64));
            result[name] = value;
            offset += 64;
        } else if (type == "string" || type == "bytes") {
            std::string offsetHex = cleanData.substr(offset, 64);
            uint64_t dynamicOffset = std::stoull(offsetHex, nullptr, 16) * 2;
            offset += 64;

            std::string value;
            if (type == "string") {
                value = decodeString(cleanData, dynamicOffset);
            } else {
                value = decodeBytes(cleanData, dynamicOffset);
            }
            result[name] = value;
        } else if (type.find("bytes") == 0 && type.length() > 5) {
            std::string valueHex = cleanData.substr(offset, 64);
            const int bytesSize = std::stoi(type.substr(5));
            std::string value = "0x" + valueHex.substr(0, bytesSize * 2);
            result[name] = value;
            offset += 64;
        } else {
            throw std::runtime_error("Unsupported type for decoding: " + type);
        }
    }

    return result;
}

// Decode padded address from 32-byte hex string
std::string Contract::decodeAddress(const std::string &paddedAddress) {
    if (paddedAddress.length() != 64) {
        throw std::runtime_error("Invalid padded address length");
    }

    return "0x" + paddedAddress.substr(24, 40);
}

// Decode unsigned integer from 32-byte hex string, supports large numbers
std::string Contract::decodeUint(const std::string &hexValue) {
    if (hexValue.length() != 64) {
        throw std::runtime_error(
            "Invalid hex value length for uint: " + std::to_string(hexValue.length()) + " (expected 64)");
    }

    if (hexValue.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        throw std::runtime_error("Invalid hex characters in uint value: " + hexValue);
    }

    try {
        if (hexValue == std::string(64, '0')) {
            return "0";
        }

        std::string cleanHex = hexValue;
        size_t firstNonZero = cleanHex.find_first_not_of('0');
        if (firstNonZero != std::string::npos) {
            cleanHex = cleanHex.substr(firstNonZero);
        } else {
            return "0";
        }

        if (cleanHex.length() > 16) {
            mpz_class bigInt(hexValue, 16);
            return bigInt.get_str();
        }

        return std::to_string(std::stoull(cleanHex, nullptr, 16));
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to decode uint from hex '" + hexValue + "': " + std::string(e.what()));
    }
}

// Decode signed integer from 32-byte hex string with two's complement
std::string Contract::decodeInt(const std::string &hexValue) {
    if (hexValue.length() != 64) {
        throw std::runtime_error(
            "Invalid hex value length for int: " + std::to_string(hexValue.length()) + " (expected 64)");
    }

    if (hexValue.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        throw std::runtime_error("Invalid hex characters in int value: " + hexValue);
    }

    try {
        if (hexValue == std::string(64, '0')) {
            return "0";
        }

        mpz_class bigInt(hexValue, 16);

        if (hexValue[0] >= '8') {
            mpz_class maxUint256;
            mpz_ui_pow_ui(maxUint256.get_mpz_t(), 2, 256);

            if (bigInt >= maxUint256 / 2) {
                bigInt -= maxUint256;
            }
        }

        return bigInt.get_str();
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to decode int from hex '" + hexValue + "': " + std::string(e.what()));
    }
}

// Decode boolean from 32-byte hex string
std::string Contract::decodeBool(const std::string &hexValue) {
    if (hexValue.length() != 64) {
        throw std::runtime_error("Invalid hex value length for bool");
    }

    return (hexValue.back() == '1') ? "true" : "false";
}

// Decode dynamic bytes from response data at given offset
std::string Contract::decodeBytes(const std::string &data, size_t offset) {
    if (offset + 64 > data.length()) {
        throw std::runtime_error("Invalid offset for bytes decoding");
    }

    size_t length = std::stoull(data.substr(offset, 64), nullptr, 16);
    offset += 64;

    if (offset + length * 2 > data.length()) {
        throw std::runtime_error("Invalid length for bytes decoding");
    }

    return "0x" + data.substr(offset, length * 2);
}

// Decode string from response data at given offset
std::string Contract::decodeString(const std::string &data, size_t offset) {
    if (offset + 64 > data.length()) {
        throw std::runtime_error("Invalid offset for string decoding");
    }

    size_t length = std::stoull(data.substr(offset, 64), nullptr, 16);
    offset += 64;

    if (offset + length * 2 > data.length()) {
        throw std::runtime_error("Invalid length for string decoding");
    }

    std::string hexString = data.substr(offset, length * 2);
    std::string result;

    for (size_t i = 0; i < hexString.length(); i += 2) {
        std::string byteString = hexString.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
        result += byte;
    }

    return result;
}
