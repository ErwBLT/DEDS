#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <gmpxx.h>
#include "../utils/Web3Client.h"

// Token structure with ERC20 metadata and global indexing
struct Token {
    std::string address;
    std::string symbol;
    std::string name;
    int decimals;
    int tokenGlobalIndice;

    void ERC20sync(const std::shared_ptr<Web3Client> &web3);
};

#endif // TOKEN_H
