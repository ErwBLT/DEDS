#include "Token.h"
#include "../utils/Contract.h"

// Sync token metadata from ERC20 contract
void Token::ERC20sync(const std::shared_ptr<Web3Client> &web3) {
    Contract contract(address, "../abis/erc20.json");
    symbol = web3->call(contract, "symbol")[""];
    name = web3->call(contract, "name")[""];
    decimals = std::stoi(web3->call(contract, "decimals")[""].get<std::string>());
}
