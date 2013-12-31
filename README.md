bitcoin-wallet-app
===

A Bitcoin Wallet Chrome App that implements a BIP0032 hierarchical
deterministic wallet.

Portions adapted from
[the reference Bitcoin client](https://github.com/bitcoin/bitcoin) and
Eric Lombrozo's [CoinClasses project](https://github.com/CodeShark/CoinClasses).

System Requirements
===

* [Chrome](https://www.google.com/chrome/)
* The [Native Client SDK](https://developers.google.com/native-client/sdk/download)
* `NACL_SDK_ROOT` pointing to the appropriate `pepper_NN` directory in the NaCl SDK
* [naclports](https://code.google.com/p/naclports/) with `./make_all.sh openssl` successfully executed on the local system (for what it's worth, I had to exclude the glibc versions to get it to build with unit tests.)
* [googletest](https://code.google.com/p/googletest/) (a.k.a. gtest)

Installation/Development
===

1. Clone the repository.
2. `make`.
3. In Chrome, load the unpacked extension in `out/zip`.

Acknowledgments (in addition to the [Bitcoin developers](https://github.com/bitcoin))
===

* [Richard Kiss](http://blog.richardkiss.com/?p=313), whose blog post walked through what a BIP0032 wallet really is.
* [Eric Lombrozo](https://github.com/CodeShark), some of whose C++ BIP0032 HD wallet code appears here.
* [JP Richardson](https://github.com/jprichardson)'s invaluable [blog post](http://procbits.com/2013/08/27/generating-a-bitcoin-address-with-javascript) that went into interactive detail about compressed keys/addresses.
* Various folks who have posted on [bitcointalk](https://bitcointalk.org/) about BIP0032.
