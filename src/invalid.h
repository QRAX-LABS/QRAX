// Copyright (c) 2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QRAX_INVALID_H
#define QRAX_INVALID_H

#include <set>
#include <string>

#include "primitives/transaction.h"
#include <univalue.h>

class COutPoint;
class UniValue;

namespace invalid_out
{
    extern std::set<COutPoint> setInvalidOutPoints;

    UniValue read_json(const std::string& jsondata);

    bool ContainsOutPoint(const COutPoint& out);
	bool ContainsAssetKey(const CKeyID &key);
    bool LoadOutpoints();
    bool LoadAddresses();
}

#endif //QRAX_INVALID_H
