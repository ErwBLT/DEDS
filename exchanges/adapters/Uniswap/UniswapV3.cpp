#include "UniswapV3.h"

#include <iostream>
#include <utility>
#include <gmpxx.h>

#include "../../../utils/Utils.h"
#include "../../../utils/Contract.h"

using json = nlohmann::json;
using string = std::string;
template<typename T>
using vector = std::vector<T>;

// Constructor: Initialize UniswapV3 exchange with pools and token data
UniswapV3::UniswapV3(std::shared_ptr<Web3Client> web3Client,int tickRange)
    : ExchangeBase(std::move(web3Client), "UniswapV3"), tickRange(tickRange) {
    pools = Utils::initPools("../data/uniswapV3.txt");
    for (auto &pool: pools | std::views::values) {
        pool->poolContract = std::make_unique<Contract>(Contract(pool->address,
                                                                 "../abis/uniswap_v3_pool.json"));
        pool->exchange = name;
        pool->fee = stod(web3->call(*pool->poolContract, "fee")[""].get<string>()) / 1e6;

        string token0Adress = web3->call(*pool->poolContract, "token0")[""];
        string token1Adress = web3->call(*pool->poolContract, "token1")[""];
        addToken(token0Adress);
        addToken(token1Adress);
        Token token0 = tokens[token0Adress];
        Token token1 = tokens[token1Adress];
        pool->tokens.push_back(std::make_unique<Token>(token0));
        pool->tokens.push_back(std::make_unique<Token>(token1));
    }
}

// Update pools with tick data using batch multicall
void UniswapV3::updatePools() {
    try {
        // STAGE 1: Batch slot0 calls for all pools
        std::vector<CallRequest> slot0Calls;
        std::vector<std::string> poolAddresses;

        for (const auto &[address, pool]: pools) {
            slot0Calls.push_back({*pool->poolContract, "slot0", json::array()});
            poolAddresses.push_back(address);
        }

        if (slot0Calls.empty()) return;

        json slot0Results = web3->multicall(slot0Calls);

        // STAGE 2: Prepare tick calls based on slot0 data
        std::vector<CallRequest> tickCalls;
        std::map<int, std::pair<std::string, int> > tickCallToPool;
        int tickCallIndex = 0;

        for (size_t i = 0; i < poolAddresses.size(); i++) {
            const auto &address = poolAddresses[i];
            auto &pool = *pools[address];
            json slot0Data = slot0Results["slot0"][i];

            int currentTick = std::stoi(slot0Data["tick"].get<std::string>());

            poolSqrtPriceX96[address] = slot0Data["sqrtPriceX96"].get<std::string>();

            // Get tickSpacing for this pool
            int tickSpacing;
            if (pool.fee == 0.0001) tickSpacing = 1;
            else if (pool.fee == 0.0005) tickSpacing = 10;
            else if (pool.fee == 0.003) tickSpacing = 60;
            else if (pool.fee == 0.01) tickSpacing = 200;
            else {
                std::cerr << "Unknown fee tier: " << pool.fee << " for pool " << address << std::endl;
                tickSpacing = 60;
            }

            // Align current tick to tick spacing boundary
            int alignedTick;
            if (currentTick >= 0) {
                alignedTick = (currentTick / tickSpacing) * tickSpacing;
            } else {
                alignedTick = ((currentTick - tickSpacing + 1) / tickSpacing) * tickSpacing;
            }

            // Calculate range around current price
            int minTick = std::max(-887272, alignedTick - tickRange * tickSpacing);
            int maxTick = std::min(887272, alignedTick + tickRange * tickSpacing);

            // Generate tick calls
            for (int tick = minTick; tick <= maxTick; tick += tickSpacing) {
                tickCalls.push_back({*pool.poolContract, "ticks", json::array({tick})});
                tickCallToPool[tickCallIndex++] = {address, tick};
            }
        }

        if (tickCalls.empty()) return;

        // STAGE 3: Execute tick multicall
        json tickResults = web3->multicall(tickCalls);

        // STAGE 4: Process results and update pools
        for (const auto &address: pools | std::views::keys) {
            std::unordered_map<int, Tick> poolTicks;

            for (int i = 0; i < tickCallIndex; i++) {
                auto [poolAddr, tick] = tickCallToPool[i];
                if (poolAddr == address) {
                    json tickData = tickResults["ticks"][i];

                    if (tickData.is_null() || tickData.empty()) continue;

                    try {
                        std::string liquidityNetStr = tickData["liquidityNet"].get<std::string>();
                        std::string liquidityGrossStr = tickData["liquidityGross"].get<std::string>();

                        if (liquidityNetStr == "0x0" && liquidityGrossStr == "0x0") continue;

                        mpf_class liquidityNetValue = 0;
                        mpf_class liquidityGrossValue = 0;

                        // Remove "0x" prefix
                        if (liquidityNetStr.substr(0, 2) == "0x") {
                            liquidityNetStr = liquidityNetStr.substr(2);
                        }
                        if (liquidityGrossStr.substr(0, 2) == "0x") {
                            liquidityGrossStr = liquidityGrossStr.substr(2);
                        }

                        // Handle negative values for liquidityNet
                        bool isNegative = false;
                        if (!liquidityNetStr.empty() && liquidityNetStr[0] == '-') {
                            isNegative = true;
                            liquidityNetStr = liquidityNetStr.substr(1);
                        }

                        // Convert hex strings using GMP
                        try {
                            mpz_class netValueMpz;
                            netValueMpz.set_str(liquidityNetStr, 16);
                            liquidityNetValue = mpf_class(netValueMpz);
                            if (isNegative) liquidityNetValue = -liquidityNetValue;

                            mpz_class grossValueMpz;
                            grossValueMpz.set_str(liquidityGrossStr, 16);
                            liquidityGrossValue = mpf_class(grossValueMpz);
                        } catch (const std::exception &e) {
                            std::cerr << "Error converting liquidity values: " << e.what() << std::endl;
                        }

                        // Create and store Tick
                        Tick tickObj;
                        tickObj.liquidity[0] = liquidityNetValue;
                        tickObj.liquidity[1] = liquidityGrossValue;
                        poolTicks[tick] = tickObj;
                    } catch (const std::exception &e) {
                        std::cerr << "Error processing tick " << tick << " for pool " << poolAddr << ": " << e.what() <<
                                std::endl;
                        continue;
                    }
                }
            }

            poolsReserves[address] = poolTicks;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error in updatePools batch operation: " << e.what() << std::endl;
    }
}
