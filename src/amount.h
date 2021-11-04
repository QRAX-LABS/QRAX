// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin developers
// Copyright (c) 2017-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QRAX_AMOUNT_H
#define QRAX_AMOUNT_H

#include <stdint.h>

/** Amount in QRAX (Can be negative) */
typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

static const CAmount MIN_FEE = 0.1 * COIN;
static const CAmount MAX_FEE = 100 * COIN;
static const double FEE_PERCENT = 0.01;

#endif //  QRAX_AMOUNT_H

