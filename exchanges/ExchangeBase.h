#ifndef EXCHANGE_BASE_H
#define EXCHANGE_BASE_H

#include <optional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>


#include "Pool.h"
#include "Token.h"
#include "../utils/Web3Client.h"

using string = std::string;
template<typename T>
using vector = std::vector<T>;

// Abstract base class for all exchange implementations
class ExchangeBase {
public:
    ExchangeBase(std::shared_ptr<Web3Client> web3Client, std::string exchangeName);

    virtual ~ExchangeBase() = default;

    // Abstract method for exchange-specific implementation
    virtual void updatePools() =0;

    void addToken(const string &address);

    std::string name;
    static std::unordered_map<std::string, Token> tokens;
    std::unordered_map<std::string, std::unique_ptr<Pool> > pools;

protected:
    std::shared_ptr<Web3Client> web3;

    static std::optional<int> getLocalIndex(const Token &token, const Pool &pool);
};

#endif // EXCHANGE_BASE_H
