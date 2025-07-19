#ifndef UNISWAP_V2_H
#define UNISWAP_V2_H

#include "../../ExchangeBase.h"
#include <memory>
#include <gmpxx.h>
#include <nlohmann/json.hpp>

template<typename T>
using vector = std::vector<T>;

// UniswapV2 exchange implementation with constant product AMM
class UniswapV2 final : public ExchangeBase {
public:
    explicit UniswapV2(std::shared_ptr<Web3Client> web3Client);

    void updatePools() override;


    std::unordered_map<std::string, std::array<mpz_class, 2> > poolsReserves;

private:
    mpf_class defaultFee;
};


#endif // UNISWAP_V2_H
