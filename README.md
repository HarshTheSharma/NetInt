# NetInt.h

This project implements a secure multi-party computation (MPC) library for C++, featuring a custom integer type (`NetInt`) and agent/server communication for secure arithmetic and comparison operations.

## Features

- Secure integer arithmetic (`NetInt`) with support for addition, subtraction, multiplication, and comparisons
- Agent/server protocol using sockets and IP whitelisting
- Easy-to-use C++ interface for secure computation that works for most programs
- Example program implementations: matrix multiplication, Dijkstra's algorithm, etc.

## Getting Started

### Build

To build the agent and sample programs:

```sh
make
```

### Library Usage

> **Note:** The order of these steps is important. Hiding messages and adding a whitelist must be done before establishing a port.

1. Move `NetInt.h` and accompanying `openssl\` folder to your project directory.
2. Import the library with `#include "NetInt.h"`.
3. **Optional:** Suppress non-error messages with `hideMessages(true);`
4. **Optional:** Create an IP address whitelist with `setWhitelist({"IP1", "IP2", ...});`
5. Open a port for agent communication with `establishPort("8081");` **before** defining any `NetInt` variables.

### Running The Program

1. Run the primary script (e.g. `./sample`)
2. Connect three agents to the primary script by running the agent programs with:
   ```sh
   ./agent <primary script ip> <port>
   ```
   If agents are on the same machine as the primary script, run:
   ```sh
   ./agent 127.0.0.1 <port>
   ```

## File Structure

- `NetInt.h` — Secure integer type and MPC context
- `agent.c` — Agent communication logic
- `sample.cpp`, `sample2.cpp`, `sample3.cpp` — Example applications
- `Makefile` — Build instructions
- `Local Standalone\` — Contains object oriented cryptographic function implementations with descriptive commenting.
- `Networked Standalone\` — Contains Networked Code in a Server-like configuration, much easier to test.

## Possible Improvements
- **Cryptologically secure random number generation:**  
  Replace rand with a suitable, preferably cross-platform alternative.
    
- **Improvements to negative number calculations, returns, and comparisons:**  
  Enhance support for negative values in secure arithmetic and comparisons.  
  Observe sample3
    
- **Addition of division:**  
  Implement secure division operations for NetInt.
    
- **Addition of modulo:**  
  Add secure modulo operations for NetInt.
    
- **Store values as arrays of shares rather than raw values:**  
  Adds more security to the primary script by storing encrypted values rather than raw data.  
  Also saves the library from having to split and reconstruct as often.  
  Drawback of having to reconstruct before returning a raw value, but calculations will be faster.

## Tips for Working on This Code
- It may be easier to work on the code by testing ideas in the Non-Networked, C version. That is included alongside a networked standalone version. Working with these is much easier than directly tackling the library code.
- The development line of Non-Networked >> Standalone >> Library makes it easier to find early implementation issues.


## Acknowledgments

- Sample code uses algorithms and code snippets from [GeeksforGeeks](https://www.geeksforgeeks.org/cpp/c-program-for-dijkstras-shortest-path-algorithm-greedy-algo-7/) and [Programiz](https://www.programiz.com/cpp-programming/examples/matrix-multiplication-function
)

- Credit to Rajais2 for foundational code
