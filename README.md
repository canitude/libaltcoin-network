[![Build Status](https://travis-ci.org/canitude/libaltcoin-network.svg?branch=version3)](https://travis-ci.org/canitude/libaltcoin-network)

[![Coverage Status](https://coveralls.io/repos/canitude/libaltcoin-network/badge.svg)](https://coveralls.io/r/canitude/libaltcoin-network)

# Libaltcoin Network

*Altcoin P2P Network Library*

Make sure you have installed [libbitcoin](https://github.com/libbitcoin/libbitcoin) beforehand according to its build instructions. 

```sh
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

libaltcoin-network is now installed in `/usr/local/`.

**Changes from Libbitcoin Network**

message_subscriber class moved to template argument. This simplifies protocol extensions.

**About Libbitcoin Network**

Libbitcoin Network is a partial implementation of the Bitcoin P2P network protocol. Excluded are all protocols that require access to a blockchain. The [libbitcoin-node](https://github.com/libbitcoin/libbitcoin-node) library extends the P2P networking capability and incorporates [libbitcoin-blockchain](https://github.com/libbitcoin/libbitcoin-blockchain) in order to implement a full node. The [libbitcoin-explorer](https://github.com/libbitcoin/libbitcoin-explorer) library uses the P2P networking capability to post transactions to the P2P network.
