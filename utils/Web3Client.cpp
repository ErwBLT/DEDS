#include "Web3Client.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cryptopp/keccak.h>
#include <cryptopp/filters.h>

using json = nlohmann::json;

// Callback function for curl to write response data
static size_t WriteCallback(void *contents, const size_t size, const size_t nmemb, std::string *s) {
    const size_t newLength{size * nmemb};
    try {
        s->append(static_cast<char *>(contents), newLength);
        return newLength;
    } catch (const std::exception &e) {
        return 0;
    }
}

// Constructor: Initialize curl globally
Web3Client::Web3Client() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

// Destructor: Clean up curl
Web3Client::~Web3Client() {
    curl_global_cleanup();
}

// Send HTTP request using curl and parse JSON response
json Web3Client::sendHttpRequest(const std::string &requestBody) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error{"Failed to initialize CURL"};
    }

    std::string responseString;
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, rpcUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    const CURLcode res = curl_easy_perform(curl);
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200 || responseString.empty()) {
        throw std::runtime_error{"HTTP request failed"};
    }

    return json::parse(responseString);
}

// Convert byte string to hexadecimal representation
std::string Web3Client::bytesToHex(const std::string &bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (const unsigned char b: bytes) {
        ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(b));
    }

    return "0x" + ss.str();
}

// Send JSON-RPC request to Ethereum node
json Web3Client::sendRpcRequest(const std::string &method, const json &params) {
    const json requestJson{
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params},
        {"id", requestId++}
    };

    json responseJson = sendHttpRequest(requestJson.dump());
    if (responseJson.contains("error")) {
        throw std::runtime_error{"RPC error: " + responseJson["error"].dump()};
    }
    return responseJson["result"];
}

// Compute Keccak-256 hash of input string
std::string Web3Client::keccak256(const std::string &input) {
    std::string digest;
    CryptoPP::Keccak_256 hash;
    CryptoPP::StringSource ss(input, true, new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(digest)));
    return digest;
}

// Get current gas price from network
mpf_class Web3Client::getGasPrice() {
    const json result = sendRpcRequest("eth_gasPrice", json::array());
    std::string hexString = result.get<std::string>();

    // Remove "0x" prefix if present
    if (hexString.substr(0, 2) == "0x") {
        hexString = hexString.substr(2);
    }

    // Convert hex to decimal string
    mpz_class gasDecimal(hexString, 16);
    return {gasDecimal};
}

// Call smart contract function and decode response
json Web3Client::call(Contract &contract, const std::string &functionName, const json &params) {
    std::string data = contract.encodeFunction(functionName, params);
    json callParams = json::array({{{"to", contract.address}, {"data", data}}, "latest"});
    json result = sendRpcRequest("eth_call", callParams);
    return contract.decodeResponse(result.get<std::string>(), functionName);
}

// Execute multiple contract calls in a single batch request
json Web3Client::multicall(std::vector<CallRequest> &calls) {
    // Create batch of individual call objects
    json batch = json::array();
    std::vector<std::pair<std::string, std::unique_ptr<Contract> > > callInfo;

    for (size_t i = 0; i < calls.size(); i++) {
        auto &call = calls[i];
        std::string data = call.contract.encodeFunction(call.functionName, call.params);

        // Create individual eth_call request
        json callRequest = {
            {"jsonrpc", "2.0"},
            {"method", "eth_call"},
            {
                "params", json::array({
                    {
                        {"to", call.contract.address},
                        {"data", data}
                    },
                    "latest"
                })
            },
            {"id", static_cast<int>(requestId + i)}
        };

        batch.push_back(callRequest);
        callInfo.emplace_back(call.functionName, std::make_unique<Contract>(call.contract));
    }

    // Send batch request using shared HTTP method
    std::string requestBody = batch.dump();
    json batchResponse = sendHttpRequest(requestBody);

    requestId += calls.size(); // Update request ID counter
    json results = json::object();

    // Process each response in the batch
    for (size_t i = 0; i < batchResponse.size(); i++) {
        const auto &response = batchResponse[i];

        if (response.contains("error")) {
            throw std::runtime_error{"RPC error in batch item " + std::to_string(i) + ": " + response["error"].dump()};
        }

        const auto &[functionName, contract] = callInfo[i];
        json result = response["result"];
        results[functionName].push_back(contract->decodeResponse(result.get<std::string>(), functionName));
    }

    return results;
}
