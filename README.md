Happynine
===

A Bitcoin Wallet Chrome App that implements a [BIP0032 hierarchical deterministic wallet](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki).

Great Big Warning
===

**This project is early and under heavy development! Do not use it for real transactions! Do not let real bitcoin anywhere near it!**

Even if/when this warning is removed, the applicable license still applies. Read it!

What Does Work
===

* Generating a new BIP0032 HD wallet
* Passphrase protection of wallet with 60-second unlock
* Viewing wallet balances
* BIP0032 account m/0'/0, which will generate keys m/0'/0/k for any 32-bit k (actually 31-bit, but nobody will get that far)
* [Robohash](http://robohash.org/) icons to help quickly confirm the right wallet is in use

What Doesn't Work Yet
===

* Sending funds
* Instant balance refresh when new transactions are detected
* Importing an external BIP0032 xprv or xpub key
* Multiple BIP0032 accounts (m/i'/0, where i is account number, 0=default). This is close to working
* Generating offline transactions
* Safety features, such as requiring a confirmation before deleting keys

Known Issues
===

* Lots of UI nits
* Refresh button doesn't do anything

System Requirements for Users
===

* [Chrome](https://www.google.com/chrome/)
* A basic understanding of [Bitcoin](http://bitcoin.org/)
* (Optional) a [Chromebook](http://www.google.com/intl/en/chrome/devices/), which is an excellent device if you're interested in avoiding Windows malware

Additional System Requirements for Development
===

* The [Native Client SDK](https://developers.google.com/native-client/sdk/download)
* `NACL_SDK_ROOT` pointing to the appropriate `pepper_NN` directory in the NaCl SDK
* [naclports](https://code.google.com/p/naclports/) with `./make_all.sh openssl` successfully executed on the local system (for what it's worth, I had to exclude the glibc versions to get it to build with unit tests.)
* [googletest](https://code.google.com/p/googletest/) (a.k.a. gtest)

Installation/Development
===

1. Clone the repository.
2. `make`.
3. In Chrome, load the unpacked extension in `out/zip`.

FAQ
===

* *What is BIP0032?* You can [read the spec](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki) if you want, but the idea is that your entire wallet can be backed up with a single 78-character string that looks like this: `xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi`. It's like [Electrum](https://electrum.org/) in this respect, but the BIP0032 spec has a few more features than Electrum's deterministic wallet.
* *Why a Chrome App?* Because [Chrome Apps](http://developer.chrome.com/apps/about_apps.html) are awesome! They're easy to write and quite secure. Security is an important reason why Chrome Apps are a good platform for anything related to Bitcoin.
* *What does "Happynine" mean?* I was looking for a catchier name than "Bitcoin Wallet App," and looked up the significance of the number 32. It turns out 32 is the [ninth happy number](http://en.wikipedia.org/wiki/Happy_number). So "happy nine." There you go.

Acknowledgments
===

* [Richard Kiss](http://blog.richardkiss.com/?p=313), whose blog post walked through what a BIP0032 wallet really is.
* [Eric Lombrozo](https://github.com/CodeShark), some of whose C++ BIP0032 HD wallet code appears here.
* [JP Richardson](https://github.com/jprichardson)'s invaluable [blog post](http://procbits.com/2013/08/27/generating-a-bitcoin-address-with-javascript) that went into interactive detail about compressed keys/addresses.
* Various folks who have posted on [bitcointalk](https://bitcointalk.org/) about BIP0032.
* Portions adapted from [the reference Bitcoin client](https://github.com/bitcoin/bitcoin).
