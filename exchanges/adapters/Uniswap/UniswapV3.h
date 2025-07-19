#ifndef UNISWAP_V3_H
#define UNISWAP_V3_H

#include "../../ExchangeBase.h"
#include <memory>

#include <nlohmann/json.hpp>

template<typename T>
using vector = std::vector<T>;

// Tick structure for UniswapV3 concentrated liquidity
struct Tick {
    std::array<mpf_class, 2> liquidity;
};

// UniswapV3 exchange implementation with concentrated liquidity
class UniswapV3 : public ExchangeBase {
public:
    explicit UniswapV3(std::shared_ptr<Web3Client> web3Client, int tickRange);

    // Implement abstract methods
    void updatePools() override;

    std::unordered_map<std::string, std::unordered_map<int, Tick> > poolsReserves;

    // Store slot0 data for each pool
    std::unordered_map<std::string, std::string> poolSqrtPriceX96;

    int tickRange;

};

#endif // UNISWAP_V3_H
