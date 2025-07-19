#include "UniswapV2.h"

#include <iostream>
#include <utility>
#include "../../../utils/Utils.h"
#include "../../../utils/Contract.h"

using json = nlohmann::json;
using string = std::string;
template<typename T>
using vector = std::vector<T>;

// Constructor: Initialize UniswapV2 exchange with pools and token data
UniswapV2::UniswapV2(std::shared_ptr<Web3Client> web3Client)
    : ExchangeBase(std::move(web3Client), "UniswapV2") {
    defaultFee = 0.997;
    pools = Utils::initPools("../data/uniswapV2.txt");

    for (auto &pool: pools | std::views::values) {
        pool->exchange = name;
        pool->fee = defaultFee;

        pool->poolContract = std::make_unique<Contract>(
            Contract(pool->address, "../abis/uniswap_v2_pair.json"));

        string token0Adress = web3->call(*pool->poolContract, "token0")[""];
        string token1Adress = web3->call(*pool->poolContract, "token1")[""];
        addToken(token0Adress);
        addToken(token1Adress);
        Token token0 = tokens[token0Adress];
        Token token1 = tokens[token1Adress];

        pool->tokens.push_back(std::make_unique<Token>(token0));
        pool->tokens.push_back(std::make_unique<Token>(token1));

        json reserves = web3->call(*pool->poolContract, "getReserves");
        mpz_class reserve0(reserves["_reserve0"].get<string>());
        mpz_class reserve1(reserves["_reserve1"].get<string>());

        poolsReserves[pool->address] = {reserve0, reserve1};
    }
}

// Update pool reserves using multicall for efficiency
void UniswapV2::updatePools() {
    try {
        std::vector<CallRequest> callRequests;
        std::vector<std::string> poolAddresses;

        for (const auto &[address, pool]: pools) {
            callRequests.push_back({*pool->poolContract, "getReserves", json::array()});
            poolAddresses.push_back(address);
        }

        if (callRequests.empty()) {
            return;
        }

        json results = web3->multicall(callRequests);

        for (size_t i = 0; i < poolAddresses.size(); i++) {
            const auto &address = poolAddresses[i];

            json reserves = results["getReserves"][i];

            mpz_class reserve0(reserves["_reserve0"].get<std::string>());
            mpz_class reserve1(reserves["_reserve1"].get<std::string>());

            poolsReserves[address] = {reserve0, reserve1};
        }
    } catch (const std::exception &e) {
        std::cerr << "Error updating Uniswap V2 pools: " << e.what() << std::endl;
    }
}
