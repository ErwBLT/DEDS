#include "ExchangeBase.h"

using string = std::string;

// Base constructor for all exchange implementations
ExchangeBase::ExchangeBase(std::shared_ptr<Web3Client> web3Client, std::string exchangeName)
    : name{std::move(exchangeName)}, web3{web3Client} {
}

// Find token index in pool's token list
std::optional<int> ExchangeBase::getLocalIndex(const Token &token, const Pool &pool) {
    for (int i = 0; i < pool.tokens.size(); i++) {
        if (pool.tokens[i]->address == token.address) {
            return i;
        }
    }
    return std::nullopt;
}

// Add token to global registry if not exists
void ExchangeBase::addToken(const string &address) {
    if (!tokens.contains(address)) {
        Token token;
        token.address = address;
        token.ERC20sync(web3);
        tokens[address] = token;
        token.tokenGlobalIndice = tokens.size() - 1;
    }
}

std::unordered_map<std::string, Token> ExchangeBase::tokens;
