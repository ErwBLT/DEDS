#ifndef POOL_H
#define POOL_H

#include <string>
#include <vector>
#include <memory>
#include <gmpxx.h>
#include "../utils/Contract.h"
#include "Token.h"

// Pool structure containing tokens, contract, and exchange information
struct Pool {
    std::string address;
    std::string exchange;
    mpf_class fee;

    std::vector<std::unique_ptr<Token> > tokens;
    std::unique_ptr<Contract> poolContract;
};

#endif
