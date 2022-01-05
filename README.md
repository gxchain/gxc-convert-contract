# GXC-Relay-Contract

This project constructs of 2 parts:

- A [Convert Contract](./contracts) deployed on REI Network
- A [Relay Contract](./gxc-contract) writen in C++ which will be deployed on GXChain

### Install dependencies

```bash
npm i truffle -g
npm install
```

### Deploy

> Note: A file named .secret should be created which contains the brain key of your eth account

```bash
npm run testnet // for rei testnet
npm run live // for rei mainnet
```

### Test

```bash
truffle console --network testnet
```
