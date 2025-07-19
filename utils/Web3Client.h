#ifndef WEB3CLIENT_H
#define WEB3CLIENT_H

#include <string>
#include <nlohmann/json.hpp>
#include "Contract.h"
#include <gmpxx.h>

using json = nlohmann::json;

// Structure for batching multiple contract calls
struct CallRequest {
    Contract contract;
    std::string functionName;
    json params;
};

// Web3 client for Ethereum JSON-RPC communication
class Web3Client {
public:
    Web3Client();

    ~Web3Client();

    // Contract interaction methods
    json call(Contract &contract, const std::string &functionName, const json &params = json::array());

    json multicall(std::vector<CallRequest> &calls);

    json sendRpcRequest(const std::string &method, const json &params = json::array());

    // Utility methods
    static std::string keccak256(const std::string &input);

    static std::string bytesToHex(const std::string &bytes);

    mpf_class getGasPrice();

private:
    std::string rpcUrl{"https://arb1.arbitrum.io/rpc"};
    unsigned int requestId{1};

    json sendHttpRequest(const std::string &requestBody);
};

#endif //WEB3CLIENT_H
