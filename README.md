Happynine
===

A Chrome App that implements a Bitcoin [BIP 0032 hierarchical deterministic wallet](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki).

Copyright &copy; 2004 Mike Tsao. All rights reserved. See LICENSE.md.

Great Big Warning
===

**This project is early and under heavy development! Do not use it for real transactions! Do not let real bitcoin anywhere near it! Even if/when this warning is removed, the applicable license still applies. Read it!**

What Does Work
===

* Generating a new BIP 0032 HD wallet
* Importing an external BIP 0032 xprv or xpub key
* Passphrase protection of wallet with 60-second unlock
* Viewing wallet balances using the [Blockchain API](https://blockchain.info/api)
* BIP 0032 account `m/0'/0`, which will generate keys `m/0'/0/k` for any 32-bit k (actually 31-bit, but nobody will get that far)
* [Robohash](http://robohash.org/) icons to help quickly confirm the right wallet is in use

What Doesn't Work Yet
===

* Sending funds
* Generating unsigned transactions for offline signing
* Instant balance refresh when new transactions are detected
* Any concept of fiat currencies (e.g., Bitstamp's USD/BTC exchange rate)
* Direct communication with the Bitcoin network; currently the app relies on Blockchain's API for all its live functionality
* Multiple BIP 0032 accounts: `m/i'/0`, where i is account number; this is close to working
* *Meaningful* BIP 0032 usage, which is the ability to install multiple master keys, some of which might turn out to be children of others
* Being able to view the xpub/xprv of an account, which is essential for controlling individual account access
* Properly treating the account chain as a chain (basically, reading up on Electrum's gap limit concept and confirming it's the right way to go)

Known Issues
===

* Lots of UI nits
* Refresh button doesn't do anything
* There are no safety features, such as requiring a confirmation before deleting keys

User Requirements
===

* A basic understanding of [Bitcoin](http://bitcoin.org/)
* [Chrome](https://www.google.com/chrome/). You don't actually have to use this as your browser, but it does need to be installed on your computer.
* (Optional) a [Chromebook](http://www.google.com/intl/en/chrome/devices/), which is an excellent device if you're interested in avoiding Windows malware. [Buy one today!](http://www.amazon.com/gp/product/B00FNPD1VW?tag=sowbug-20)

BIP 0032 Overview
===

A *root master key* is the starting point. It's the complete identity of a BIP 0032 wallet. The *xprv* version can generate the *xpub*, but not vice-versa.

The root master key xprv is used to generate master keys for *accounts*. A BIP 0032 account is what people usually think of as a Bitcoin wallet. It's a long (2.1 billion) chain of Bitcoin *addresses*. If you have an account xprv, you can generate the chain of Bitcoin addresses and keys. If you have the account's xpub, you can generate only the addresses.

The simplest BIP 0032 use case: one account, one user. Generate the root master key and account zero. Use that single chain of addresses as the user's wallet.

A more interesting use case: one family of parents Alice and Bob with kids Carol, Dan, and Eve. The parents generate the root master key and back up the xprv (for example by printing it out, putting a paper copy in a safe deposit box at a bank, and giving another paper copy to the trusted family attorney). Using the root master key, the parents generate many accounts: a spending account for each of the five people in the family, a joint spending account for the parents, and a savings account for each kid. That's a total of nine accounts. Now, Mom and Dad distribute the account master keys as follows:

* Each family member gets his or her own spending account's xprv.
* Each parent gets the joint account's xprv.
* The family attorney is instructed to disclose each child's savings account's xprv to the child on that child's 18th birthday.
* The grandparents get three Bitcoin addresses, one from each of the kids' savings xpubs.
* Aunt Olive gets three different Bitcoin addresses for the kids' savings.
* The parents keep all the xpubs, including particularly the kids' spending xpubs.
* The parents do not keep the master key xpub/xprv online.

This gives the family the following abilities:

* Each kid has complete control over his or her own savings account (because he or she has the xprv). Same with each parent. As with normal Bitcoin wallets, the kid can give out any Bitcoin address from that account to anyone, and spend the funds in any address belonging to that account.
* The parents can monitor the kids' savings accounts for unauthorized spending sprees.
* Extended family can give gifts straight to each kids' savings account (because they have Bitcoin addresses belonging to those accounts), but the kids can't spend it until their 18th birthday.
* Because the grandparents and Aunt Olive got different addresses, they can't spy on how generous the others are being. Because they have only addresses and not xpubs, they cannot spy on the overall savings balance of any of the kids.
* Eve (who, being named Eve, is cryptologically likely to be evil) cannot steal from her siblings, monitor their balances, or eavesdrop on their transactions.
* The parents can each spend from the joint account, but nobody else can.
* When the parents need to prove assets for a boat loan, they supply their joint xpub to their banker, who can easily verify the balance.
* If the family's house burns down, they can go to the bank, get the root master xprv, regenerate all the account keys, and get back all their bitcoin.

There are other ways to slice and dice a BIP 0032 tree. Another option in this scenario is to generate only one first-level child account for each person, then to generate further children (savings/spending) from each of those. One might also wonder why the parents have no savings account. Are they planning on living off the kids during retirement? Anyway, it's all up to you.

FAQ
===

* **What is BIP 0032?** You can [read the spec](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki) if you want, but the idea is that your entire wallet can be backed up and restored with a single string that looks like this: `xprv9s21ZrQH143K3QTDL4LXw2FKmP...GJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi` (the middle is elided, but it's the root of BIP 0032 Test Vector 1). The idea is similar to [Electrum](https://electrum.org/) in this respect, but the BIP 0032 spec has a few more features than Electrum's deterministic wallet. By the way, anyone else can **steal all your bitcoin** if they get that xpub string, so keep it private!
* **Can I watch a BIP 0032 wallet without enabling the app to spend any of the funds in it?** Yes, this is one of the coolest features of most deterministic wallets. If you install the xpub version of one of your accounts, then the app can view all the Bitcoin addresses but cannot generate the corresponding private keys. In English, this means you can watch the balances and transactions on an account but can't spend any of the funds in any of the addresses.
* **If I start using Happynine, am I committed to using it for my Bitcoin forever?** No, not at all. That's what's nice about BIP 0032: it's a standard. As long as you know your wallet's xprv, you can use any BIP 0032 app to manage your bitcoin. Even if there were no BIP 0032 apps but you were good at math and had a good calculator, you could derive all your Bitcoin private keys from the xprv.
* **That last answer disturbed me. Calculator? Math?** OK, how about this: if you want to switch to another wallet app, send your funds in your Happynine wallet to that one.
* **What about my vanity addresses? How do I generate a BIP 0032 vanity address?** You lose that with BIP 0032. In theory you could walk a chain until you find an address that matches the pattern you want, but then you'd have to remember the index of that address if you ever wanted to recover your wallet. It's not impossible, just not a good fit for how this kind of wallet works. In exchange for the comfort of knowing your wallet is extremely easy to back up and recover, you lose the ability to pick your own addresses.
* **I have a tips address that I have published far and wide. Can I import it into this wallet?** See previous question. You'll have to manage that address separately. It would be possible to take an approach like Electrum's, which is deterministic but adds the ability to add other addresses, but then the elegance of the simple backup is lost. At this point it's not on the project radar.
* **Why a Chrome App?** Because [Chrome Apps](http://developer.chrome.com/apps/about_apps.html) are awesome! They're easy to write and very secure. Security is an important reason why Chrome Apps are a good platform for anything related to Bitcoin, as you'd be sad if [malware stole your money](https://bitcointalk.org/index.php?topic=83794.0).
* **What does "Happynine" mean?** I was looking for a catchier name than "Bitcoin Wallet App." Hours later, I looked up the significance of the number 32 from BIP 0032. It turns out 32 is the [ninth happy number](http://en.wikipedia.org/wiki/Happy_number). So "happy nine."

Acknowledgments
===

* [Richard Kiss](http://blog.richardkiss.com/?p=313), whose blog post walked through what a BIP 0032 wallet really is.
* [Eric Lombrozo](https://github.com/CodeShark), some of whose C++ BIP 0032 HD wallet code appears here.
* [JP Richardson](https://github.com/jprichardson)'s invaluable [blog post](http://procbits.com/2013/08/27/generating-a-bitcoin-address-with-javascript) that went into interactive detail about compressed keys/addresses.
* Various folks who have posted on [bitcointalk](https://bitcointalk.org/) about BIP 0032.
* Portions adapted from [the reference Bitcoin client](https://github.com/bitcoin/bitcoin).

Begging
-

Send tips to [1BUGzQ7CiHF2FUxHVH2LbUx1oNNN9VnuC1](https://blockchain.info/address/1BUGzQ7CiHF2FUxHVH2LbUx1oNNN9VnuC1). `TODO(miket): replace with a BIP 0032 address!`
