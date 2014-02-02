// Copyright 2014 Mike Tsao <mike@sowbug.com>

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#if !defined(__TEST_CONSTANTS_H__)
#define __TEST_CONSTANTS_H__

#include <string>

#include "base58.h"
#include "types.h"

// Addresses
const std::string ADDR_12c6_B58("12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX");
const bytes_t ADDR_12c6(Base58::fromAddress(ADDR_12c6_B58));
const std::string ADDR_1A1z_B58("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa");
const bytes_t ADDR_1A1z(Base58::fromAddress(ADDR_1A1z_B58));
const std::string ADDR_1Guw_B58("1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd");
const bytes_t ADDR_1Guw(Base58::fromAddress(ADDR_1Guw_B58));
const std::string ADDR_1PB8_B58("1PB8bTcRXz1u84Yxn5JpRXDUhXwc7DxUt1");
const bytes_t ADDR_1PB8(Base58::fromAddress(ADDR_1PB8_B58));

// Transactions

const std::string TX_0E3E_HEX("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac00000000");
const bytes_t TX_0E3E(unhexlify(TX_0E3E_HEX));
const std::string TX_0E3E_HASH_HEX("0e3e2357e806b6cdb1f70b54c3a3a17b6714ee1f0e68bebb44a74b1efd512098");
const bytes_t TX_0E3E_HASH(unhexlify(TX_0E3E_HASH_HEX));

// 199TSaKH54KeWDm5cs7r43oe1ccaxVrBgC - 0.00029
const std::string TX_1BCB_HEX("01000000018498a6567575912c5b891afa51d028b250465c2423fafa121b7dfe8c9382ded3000000008b48304502207a9e02fba54f78c220ef1d3c9c2e40f49b042a3e00c607313397d02109d9907d022100f87cbf506772763cf6a5b8cd63ec2d9c574bc956af892f0d87a93b339f115b03014104c3ff3d7202a81877b8537ed836529269b79ce245d69aaf52907514cb412bbb93bf61e66a72dba22064757236063cd9ddd2094e9356bc62e955ea7752e7aa5b7bffffffff0148710000000000001976a914595a67df1963dc16c5567abdd4a6443c82780d1688ac00000000");
const bytes_t TX_1BCB(unhexlify(TX_1BCB_HEX));
const std::string TX_1BCB_HASH_HEX("1bcbf3b8244b25e4430d2abf706f5f53a16ad8ff2a42129fa9ca79477b905fbd");
const bytes_t TX_1BCB_HASH(unhexlify(TX_1BCB_HASH_HEX));

// 1PB8bTcRXz1u84Yxn5JpRXDUhXwc7DxUt1 - 0.00014 BTC (not in wallet)
// 1GuwtbNdTBeXL8ZdjHSV69MeERtwQsgLZd - 0.00014 BTC (in wallet)
const std::string TX_100D_HEX("0100000001bd5f907b4779caa99f12422affd86aa1535f6f70bf2a0d43e4254b24b8f3cb1b000000006a47304402201ede3d04b7a6c22aec5421fc089e464ce3bafc012f40d240107bf1d19be1a410022027b157c524c3211528ed32f1ec3a971a0cffe173b0b91c2c80146987a37ddbfe012103a434f5b4f9d99a4c786a44dd50d5b7832ec417ae7150f049904e3a0f544621a2ffffffff02b0360000000000001976a914f33d441fd850487267ed7681b19550761bf1e4cd88acb0360000000000001976a914ae8d5613d9e7e7281451c0abf5424a3e4295fc5088ac00000000");
const bytes_t TX_100D(unhexlify(TX_100D_HEX));
const std::string TX_100D_HASH_HEX("100dc580584e9b709fd42be707b6eaf8205efebb9b5356affbc311799be1beaf");
const bytes_t TX_100D_HASH(unhexlify(TX_100D_HASH_HEX));

// 1PB8bTcRXz1u84Yxn5JpRXDUhXwc7DxUt1 - 0.00013 BTC (not in wallet)
const std::string TX_BFB1_HEX("0100000001afbee19b7911c3fbaf56539bbbfe5e20f8eab607e72bd49f709b4e5880c50d10010000006a4730440220587d3b48f32794177b4252c2d03b10db7962934179e28eaea96edb3cff51290e022061c59b6bf2f11a69acab975b718696cddc71534c89425485a2ed3d36312aeed3012102c372ba6e50d79c1fa02a32a22d0350b176935a78fd75c134e246c9ac25c98a31ffffffff01c8320000000000001976a914f33d441fd850487267ed7681b19550761bf1e4cd88ac00000000");
const bytes_t TX_BFB1(unhexlify(TX_BFB1_HEX));
const std::string TX_BFB1_HASH_HEX("bfb1902cde25217359f5f6ab4fc9d408001d92e07b9dc2c29c41d675599f5d51");
const bytes_t TX_BFB1_HASH(unhexlify(TX_BFB1_HASH_HEX));

#endif  // #if !defined(__TEST_CONSTANTS_H__)
