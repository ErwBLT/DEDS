#include <iostream>
#include <string>
#include <memory>
#include <gmpxx.h>


#include "utils/Web3Client.h"
#include "utils/Contract.h"
#include "exchanges/adapters/Uniswap/UniswapV2.h"
#include "exchanges/adapters/Uniswap/UniswapV3.h"

using json = nlohmann::json;

// Test Web3Client and Contract functionality
bool testWeb3ClientContract() {
    std::cout << "=== Testing Web3Client + Contract ===\n";

    try {
        auto web3 = std::make_shared<Web3Client>();

        auto blockNumber = web3->sendRpcRequest("eth_blockNumber");
        std::cout << "Latest block: " << blockNumber.get<std::string>() << "\n";

        std::string usdcAddress = "0xaf88d065e77c8cC2239327C5EDb3A432268e5831";
        Contract usdc(usdcAddress, "../abis/erc20.json");

        auto nameResult = web3->call(usdc, "name");
        auto symbolResult = web3->call(usdc, "symbol");
        auto decimalsResult = web3->call(usdc, "decimals");

        std::cout << "Token name: " << nameResult[""].get<std::string>() << "\n";
        std::cout << "Token symbol: " << symbolResult[""].get<std::string>() << "\n";
        std::cout << "Token decimals: " << decimalsResult[""].get<std::string>() << "\n";

        mpf_class gasPrice = web3->getGasPrice();

        // Convert from wei to gwei for readability
        mpf_class gwei("1000000000"); // 1 gwei = 10^9 wei
        mpf_class gasPriceGwei = gasPrice / gwei;

        std::cout << "Current gas price: " << gasPriceGwei << " gwei\n";

        // Calculate gas cost for a simple transaction (21,000 gas)
        mpf_class basicTxGas("21000");
        mpf_class txCostWei = basicTxGas * gasPrice;

        // Convert to ETH
        mpf_class wei_per_eth("1000000000000000000"); // 1 ETH = 10^18 wei
        mpf_class txCostEth = txCostWei / wei_per_eth;

        std::cout << "Cost for basic transaction: " << gasPriceGwei * basicTxGas << " gwei\n";
        std::cout << "Cost for basic transaction: " << txCostEth << " ETH\n";

        std::cout << "Web3Client + Contract tests passed\n\n";
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Web3Client + Contract test failed: " << e.what() << "\n\n";
        return false;
    }
}

// Test Uniswap V2 exchange functionality
bool testUniswapV2() {
    std::cout << "=== Testing Uniswap V2 ===\n";

    try {
        auto web3 = std::make_shared<Web3Client>();
        UniswapV2 uniV2(web3);

        uniV2.updatePools();
        std::cout << "Loaded " << uniV2.pools.size() << " V2 pools\n";

        if (!uniV2.pools.empty()) {
            // Sample first 3 pools
            int count = 0;
            for (const auto &[poolAddress, pool]: uniV2.pools) {
                if (count >= 3) break;

                std::cout << "Pool " << (count + 1) << ": " << poolAddress.substr(0, 10) << "...\n";
                if (pool->tokens.size() >= 2) {
                    std::cout << "  Pair: " << pool->tokens[0]->symbol << "/" << pool->tokens[1]->symbol << "\n";
                    std::cout << "  Fee: " << pool->fee << "\n";

                    // Calculate price from reserves
                    auto reservesIt = uniV2.poolsReserves.find(poolAddress);
                    if (reservesIt != uniV2.poolsReserves.end() && reservesIt->second.size() >= 2) {
                        mpf_class reserve0(reservesIt->second[0]);
                        mpf_class reserve1(reservesIt->second[1]);

                        // Decimal adjustment
                        mpf_class divisor0 = 1;
                        mpf_class divisor1 = 1;
                        for (int i = 0; i < pool->tokens[0]->decimals; i++) divisor0 *= 10;
                        for (int i = 0; i < pool->tokens[1]->decimals; i++) divisor1 *= 10;

                        mpf_class adjustedReserve0 = reserve0 / divisor0;
                        mpf_class adjustedReserve1 = reserve1 / divisor1;

                        if (adjustedReserve0 > 0) {
                            mpf_class price = adjustedReserve1 / adjustedReserve0;
                            std::cout << "  Price: " << price << " " << pool->tokens[1]->symbol
                                    << " per " << pool->tokens[0]->symbol << "\n";
                        }
                    } else {
                        std::cout << "  Price: No reserve data\n";
                    }
                }
                count++;
            }

            std::cout << "Pools with reserves: " << uniV2.poolsReserves.size() << "\n";
        }

        std::cout << "Uniswap V2 tests passed\n\n";
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Uniswap V2 test failed: " << e.what() << "\n\n";
        return false;
    }
}

// Test Uniswap V3 exchange functionality
bool testUniswapV3() {
    std::cout << "=== Testing Uniswap V3 ===\n";

    try {
        auto web3 = std::make_shared<Web3Client>();
        UniswapV3 uniV3(web3,5); // Using a tick range of 5 around current tick for testing

        uniV3.updatePools();
        std::cout << "Loaded " << uniV3.pools.size() << " V3 pools\n";

        if (!uniV3.pools.empty()) {
            // Sample first 3 pools
            int count = 0;
            for (const auto &[poolAddress, pool]: uniV3.pools) {
                if (count >= 3) break;

                std::cout << "Pool " << (count + 1) << ": " << poolAddress.substr(0, 10) << "...\n";
                std::cout << "  Exchange: " << pool->exchange << "\n";
                std::cout << "  Fee: " << pool->fee << "\n";

                // Token pair info
                if (pool->tokens.size() >= 2) {
                    std::cout << "  Pair: " << pool->tokens[0]->symbol << "/" << pool->tokens[1]->symbol << "\n";
                }

                // Tick data
                auto poolReservesIt = uniV3.poolsReserves.find(poolAddress);
                if (poolReservesIt != uniV3.poolsReserves.end() && !poolReservesIt->second.empty()) {
                    std::cout << "  Ticks: " << poolReservesIt->second.size() << "\n";

                    // Display sqrt price data
                    auto sqrtPriceIt = uniV3.poolSqrtPriceX96.find(poolAddress);
                    if (sqrtPriceIt != uniV3.poolSqrtPriceX96.end()) {
                        std::cout << "  SqrtPrice96: " << sqrtPriceIt->second << "\n";
                    } else {
                        std::cout << "  SqrtPrice96: Not available\n";
                    }
                } else {
                    std::cout << "  Ticks: 0\n";
                }
                count++;
            }

            // Pool statistics
            std::cout << "Pools with tick data: " << uniV3.poolsReserves.size() << "\n";
            if (!uniV3.poolSqrtPriceX96.empty()) {
                std::cout << "Pools with sqrt price data: " << uniV3.poolSqrtPriceX96.size() << "\n";
            }
        }

        std::cout << "Uniswap V3 tests passed\n\n";
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Uniswap V3 test failed: " << e.what() << "\n\n";
        return false;
    }
}

// Main function - run all tests
int main() {
    int passed = 0;
    if (testWeb3ClientContract()) {
        passed++;
    }
    if (testUniswapV2()) {
        passed++;
    }
    if (testUniswapV3()) {
        passed++;
    }

    return passed;
}
