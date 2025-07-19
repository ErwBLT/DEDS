# DEDS - Decentralized Exchange Data Scraper

A high-performance C++ library for interacting with EVM-compatible blockchains and efficiently loading data from decentralized exchanges (DEXs). Currently configured for Arbitrum blockchain with support for Uniswap V2 and V3 protocols.

## Features

### Core Web3 Functionality
- **EVM/Smart Contract Interaction**: Complete implementation for calling smart contract functions
- **ABI Encoding/Decoding**: Full support for Ethereum ABI encoding and decoding
- **Multicall Support**: Batch multiple contract calls for maximum efficiency
- **High-Precision Mathematics**: Built-in support for large numbers using GMP library
- **JSON-RPC Client**: Native Ethereum JSON-RPC communication

### DEX Integration
- **Uniswap V2**: Complete implementation for constant product AMM pools
- **Uniswap V3**: Advanced concentrated liquidity support with tick data
- **Efficient Data Loading**: Optimized batch calls to minimize RPC requests
- **Real-time Price Calculation**: Automatic price derivation from pool reserves
- **Token Metadata**: Automatic ERC20 token information synchronization

## Dependencies

- **C++20** or later
- **CMake 3.30+**
- **CURL** - HTTP client for RPC communication
- **nlohmann/json** - JSON parsing and manipulation
- **CryptoPP** - Cryptographic operations (Keccak-256)
- **GMP/GMPXX** - High-precision arithmetic for large numbers

## Installation

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev nlohmann-json3-dev libcrypto++-dev libgmp-dev
```

### Build the Project
```bash
mkdir build && cd build
cmake ..
make
```

## Project Structure

```
DEDS/
├── utils/                    # Core Web3 utilities
│   ├── Web3Client.h/cpp     # JSON-RPC client for blockchain communication
│   ├── Contract.h/cpp       # Smart contract ABI encoding/decoding
│   └── Utils.h/cpp          # File operations and utilities
├── exchanges/               # DEX implementations
│   ├── ExchangeBase.h/cpp   # Abstract base class for exchanges
│   ├── Pool.h               # Pool data structure
│   ├── Token.h/cpp          # ERC20 token representation
│   └── adapters/
│       └── Uniswap/
│           ├── UniswapV2.h/cpp  # Uniswap V2 implementation
│           └── UniswapV3.h/cpp  # Uniswap V3 implementation
├── abis/                    # Smart contract ABIs
├── data/                    # Pool address lists
└── main.cpp                 # Test suite and usage examples
```

## Usage Examples

The `main.cpp` file contains comprehensive tests demonstrating all library features:

### Basic Web3 Operations

```cpp
#include "utils/Web3Client.h"
#include "utils/Contract.h"

// Initialize Web3 client (configured for Arbitrum)
auto web3 = std::make_shared<Web3Client>();

// Get latest block number
auto blockNumber = web3->sendRpcRequest("eth_blockNumber");
std::cout << "Latest block: " << blockNumber.get<std::string>() << "\n";

// Interact with ERC20 contract
std::string usdcAddress = "0xaf88d065e77c8cC2239327C5EDb3A432268e5831";
Contract usdc(usdcAddress, "../abis/erc20.json");

auto nameResult = web3->call(usdc, "name");
auto symbolResult = web3->call(usdc, "symbol");
auto decimalsResult = web3->call(usdc, "decimals");

std::cout << "Token: " << nameResult[""].get<std::string>() << " (" 
          << symbolResult[""].get<std::string>() << ")\n";
```

### Uniswap V2 Integration

```cpp
#include "exchanges/adapters/Uniswap/UniswapV2.h"

// Initialize Uniswap V2 exchange
auto web3 = std::make_shared<Web3Client>();
UniswapV2 uniV2(web3);

// Load and update all pools efficiently
uniV2.updatePools();
std::cout << "Loaded " << uniV2.pools.size() << " V2 pools\n";

// Access pool data
for (const auto &[poolAddress, pool] : uniV2.pools) {
    if (pool->tokens.size() >= 2) {
        std::cout << "Pair: " << pool->tokens[0]->symbol 
                  << "/" << pool->tokens[1]->symbol << "\n";
        
        // Get reserves and calculate price
        auto reservesIt = uniV2.poolsReserves.find(poolAddress);
        if (reservesIt != uniV2.poolsReserves.end()) {
            mpf_class reserve0(reservesIt->second[0]);
            mpf_class reserve1(reservesIt->second[1]);
            // Price calculation with decimal adjustment...
        }
    }
}
```

### Uniswap V3 Advanced Features

```cpp
#include "exchanges/adapters/Uniswap/UniswapV3.h"

// Initialize Uniswap V3 exchange
UniswapV3 uniV3(web3);

// Load pools with tick data
uniV3.updatePools();
std::cout << "Loaded " << uniV3.pools.size() << " V3 pools\n";

// Access concentrated liquidity data
for (const auto &[poolAddress, pool] : uniV3.pools) {
    std::cout << "Pool: " << poolAddress.substr(0, 10) << "...\n";
    std::cout << "Fee: " << pool->fee << "\n";
    
    // Access tick data
    auto tickDataIt = uniV3.poolsReserves.find(poolAddress);
    if (tickDataIt != uniV3.poolsReserves.end()) {
        std::cout << "Active ticks: " << tickDataIt->second.size() << "\n";
    }
    
    // Get current sqrt price
    auto sqrtPriceIt = uniV3.poolSqrtPriceX96.find(poolAddress);
    if (sqrtPriceIt != uniV3.poolSqrtPriceX96.end()) {
        std::cout << "SqrtPriceX96: " << sqrtPriceIt->second << "\n";
    }
}
```

### Efficient Batch Operations

```cpp
// Multicall for efficient batch operations
std::vector<CallRequest> callRequests;

for (const auto &[address, pool] : pools) {
    callRequests.push_back({*pool->poolContract, "getReserves", json::array()});
}

// Execute all calls in a single RPC request
json results = web3->multicall(callRequests);
```

## Running Tests

The project includes comprehensive tests in `main.cpp`:

```bash
# Build and run tests
cd cmake-build-debug
./DEDS
```

The test suite covers:
1. **Web3Client + Contract functionality** - Basic blockchain interaction
2. **Uniswap V2 operations** - Pool loading and price calculation
3. **Uniswap V3 operations** - Tick data and concentrated liquidity

## Configuration

### Blockchain Configuration
Currently configured for **Arbitrum** mainnet:
- RPC URL: `https://arb1.arbitrum.io/rpc`
- Chain ID: 42161

To change blockchain, modify the `rpcUrl` in `Web3Client.cpp`:

```cpp
std::string rpcUrl{"https://your-rpc-endpoint.com"};
```

### Pool Data Sources
Pool addresses are loaded from text files:
- `data/uniswapV2.txt` - Uniswap V2 pool addresses
- `data/uniswapV3.txt` - Uniswap V3 pool addresses

### ABI Files
Smart contract ABIs are stored in `abis/`:
- `erc20.json` - Standard ERC20 token interface
- `uniswap_v2_pair.json` - Uniswap V2 pair contract
- `uniswap_v3_pool.json` - Uniswap V3 pool contract

## Performance Features

- **Batch Operations**: Multicall support reduces RPC calls by up to 100x
- **Memory Efficient**: Smart pointer usage and optimized data structures
- **High Precision**: GMP library handles large numbers without precision loss
- **Concurrent Safe**: Thread-safe design for multi-threaded applications
- **Minimal Dependencies**: Lightweight with essential libraries only

## Advanced Usage

### Custom Contract Integration

```cpp
// Load custom contract ABI
Contract customContract("0x...", "path/to/custom.json");

// Call custom functions
json result = web3->call(customContract, "customFunction", json::array({param1, param2}));
```

### Gas Price Monitoring

```cpp
mpf_class gasPrice = web3->getGasPrice();
mpf_class gasPriceGwei = gasPrice / mpf_class("1000000000");
std::cout << "Current gas price: " << gasPriceGwei << " gwei\n";
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

---

**Note**: This library is optimized for Arbitrum but can be easily adapted for other EVM-compatible chains by changing the RPC endpoint and updating contract addresses.
