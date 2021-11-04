// Copyright (c) 2021 The QRAX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SPENTINDEX_H
#define BITCOIN_SPENTINDEX_H

#include <uint256.h>
#include <amount.h>
#include <script/script.h>
#include <serialize.h>
#include <chainparams.h>

struct CSpentIndexKey {
    uint256 txid;
    unsigned int outputIndex;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(txid);
        READWRITE(outputIndex);
    }

    CSpentIndexKey(uint256 t, unsigned int i) {
        txid = t;
        outputIndex = i;
    }

    CSpentIndexKey() {
        SetNull();
    }

    void SetNull() {
        txid.SetNull();
        outputIndex = 0;
    }

};

struct CSpentIndexValue {
    uint256 txid;
    unsigned int inputIndex;
    int blockHeight;
    CAmount satoshis;
    int16_t addressType;
    uint160 addressHash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(txid);
        READWRITE(inputIndex);
        READWRITE(blockHeight);
        READWRITE(satoshis);
        READWRITE(addressType);
        READWRITE(addressHash);
    }

    CSpentIndexValue(uint256 t, unsigned int i, int h, CAmount s, CChainParams::Base58Type type, uint160 a) {
        txid = t;
        inputIndex = i;
        blockHeight = h;
        satoshis = s;
        addressType = type;
        addressHash = a;
    }

    CSpentIndexValue() {
        SetNull();
    }

    void SetNull() {
        txid.SetNull();
        inputIndex = 0;
        blockHeight = 0;
        satoshis = 0;
        addressType = CChainParams::Base58Type::UNKNOWN_TYPE;
        addressHash.SetNull();
    }

    bool IsNull() const {
        return txid.IsNull();
    }
};

struct CSpentIndexKeyCompare
{
    bool operator()(const CSpentIndexKey& a, const CSpentIndexKey& b) const {
        if (a.txid == b.txid) {
            return a.outputIndex < b.outputIndex;
        } else {
            return a.txid < b.txid;
        }
    }
};

struct CSpentIndexTxInfo
{
    std::map<CSpentIndexKey, CSpentIndexValue, CSpentIndexKeyCompare> mSpentInfo;
};

struct CTimestampIndexIteratorKey {
    unsigned int timestamp;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 4;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata32be(s, timestamp);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        timestamp = ser_readdata32be(s);
    }

    CTimestampIndexIteratorKey(unsigned int time) {
        timestamp = time;
    }

    CTimestampIndexIteratorKey() {
        SetNull();
    }

    void SetNull() {
        timestamp = 0;
    }
};

struct CTimestampIndexKey {
    unsigned int timestamp;
    uint256 blockHash;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 36;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata32be(s, timestamp);
        blockHash.Serialize(s);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        timestamp = ser_readdata32be(s);
        blockHash.Unserialize(s);
    }

    CTimestampIndexKey(unsigned int time, uint256 hash) {
        timestamp = time;
        blockHash = hash;
    }

    CTimestampIndexKey() {
        SetNull();
    }

    void SetNull() {
        timestamp = 0;
        blockHash.SetNull();
    }
};

struct CAddressUnspentKey {
    int16_t type;
    uint160 hashBytes;
    uint256 txhash;
    size_t index;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 57;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata16(s, type);
        hashBytes.Serialize(s);
        txhash.Serialize(s);
        ser_writedata32(s, index);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata16(s);
        hashBytes.Unserialize(s);
        txhash.Unserialize(s);
        index = ser_readdata32(s);
    }

    CAddressUnspentKey(CChainParams::Base58Type addressType, uint160 addressHash, uint256 txid, size_t indexValue) {
        type = addressType;
        hashBytes = addressHash;
        txhash = txid;
        index = indexValue;
    }

    CAddressUnspentKey() {
        SetNull();
    }

    void SetNull() {
        type = -1;
        hashBytes.SetNull();
        txhash.SetNull();
        index = 0;
    }
};

struct CAddressUnspentValue {
    CAmount satoshis;
    CScript script;
    int blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(satoshis);
        READWRITE(script);
        READWRITE(blockHeight);
    }

    CAddressUnspentValue(CAmount sats, CScript scriptPubKey, int height) {
        satoshis = sats;
        script = scriptPubKey;
        blockHeight = height;
    }

    CAddressUnspentValue() {
        SetNull();
    }

    void SetNull() {
        satoshis = -1;
        script.clear();
        blockHeight = 0;
    }

    bool IsNull() const {
        return (satoshis == -1);
    }
};

struct CAddressIndexKey {
    int16_t type;
    uint160 hashBytes;
    int blockHeight;
    unsigned int txindex;
    uint256 txhash;
    size_t index;
    bool spending;
    bool delegated;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 66;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata16(s, type);
        hashBytes.Serialize(s);
        // Heights are stored big-endian for key sorting in LevelDB
        ser_writedata32be(s, blockHeight);
        ser_writedata32be(s, txindex);
        txhash.Serialize(s);
        ser_writedata32(s, index);
        char f = spending;
        ser_writedata8(s, f);
        char d = delegated;
        ser_writedata8(s, d);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata16(s);
        hashBytes.Unserialize(s);
        blockHeight = ser_readdata32be(s);
        txindex = ser_readdata32be(s);
        txhash.Unserialize(s);
        index = ser_readdata32(s);
        char f = ser_readdata8(s);
        spending = f;
        char d = ser_readdata8(s);
        delegated = d;
    }

    CAddressIndexKey(CChainParams::Base58Type addressType, uint160 addressHash, int height, int blockindex,
                     uint256 txid, size_t indexValue, bool isSpending, bool isDelegated) {
        type = addressType;
        hashBytes = addressHash;
        blockHeight = height;
        txindex = blockindex;
        txhash = txid;
        index = indexValue;
        spending = isSpending;
        delegated = isDelegated;
    }

    CAddressIndexKey() {
        SetNull();
    }

    void SetNull() {
        type = -1;
        hashBytes.SetNull();
        blockHeight = 0;
        txindex = 0;
        txhash.SetNull();
        index = 0;
        spending = false;
        delegated = false;
    }

};


struct CAddressDelegationUnspentKey {
	uint160 stakerHash;
	uint160 ownerHash;
	uint256 txhash;
	size_t index;

	size_t GetSerializeSize(int nType, int nVersion) const {
		return 76;
	}
	template<typename Stream>
	void Serialize(Stream& s) const {
		stakerHash.Serialize(s);
		ownerHash.Serialize(s);
		txhash.Serialize(s);
		ser_writedata32(s, index);
	}
	template<typename Stream>
	void Unserialize(Stream& s) {
		stakerHash.Unserialize(s);
		ownerHash.Unserialize(s);
		txhash.Unserialize(s);
		index = ser_readdata32(s);
	}

	CAddressDelegationUnspentKey(uint160 addressStaker, uint160 addressOwner, uint256 txid, size_t indexValue) {
		stakerHash = addressStaker;
		ownerHash = addressOwner;
		txhash = txid;
		index = indexValue;
	}

	CAddressDelegationUnspentKey() {
		SetNull();
	}

	void SetNull() {
		stakerHash.SetNull();
		ownerHash.SetNull();
		txhash.SetNull();
		index = 0;
	}
};

struct CAddressDelegationIndexKey {
	uint160 stakerHash;
	uint160 ownerHash;
	int blockHeight;
	unsigned int txindex;
	uint256 txhash;
	size_t index;
	bool spending;

	size_t GetSerializeSize(int nType, int nVersion) const {
		return 85;
	}

	template<typename Stream>
	void Serialize(Stream& s) const {
		stakerHash.Serialize(s);
		ownerHash.Serialize(s);
		// Heights are stored big-endian for key sorting in LevelDB
		ser_writedata32be(s, blockHeight);
		ser_writedata32be(s, txindex);
		txhash.Serialize(s);
		ser_writedata32(s, index);
		char f = spending;
		ser_writedata8(s, f);
	}

	template<typename Stream>
	void Unserialize(Stream& s) {
		stakerHash.Unserialize(s);
		ownerHash.Unserialize(s);
		blockHeight = ser_readdata32be(s);
		txindex = ser_readdata32be(s);
		txhash.Unserialize(s);
		index = ser_readdata32(s);
		char f = ser_readdata8(s);
		spending = f;
	}

	CAddressDelegationIndexKey(uint160 addressStaker, uint160 addressOwner, int height, int blockindex,
					 uint256 txid, size_t indexValue, bool isSpending) {
		stakerHash = addressStaker;
		ownerHash = addressOwner;
		blockHeight = height;
		txindex = blockindex;
		txhash = txid;
		index = indexValue;
		spending = isSpending;
	}

	CAddressDelegationIndexKey() {
		SetNull();
	}

	void SetNull() {
		stakerHash.SetNull();
		ownerHash.SetNull();
		blockHeight = 0;
		txindex = 0;
		txhash.SetNull();
		index = 0;
		spending = false;
	}

};

struct CAddressIndexIteratorKey {
    int16_t type;
    uint160 hashBytes;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 21;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata16(s, type);
        hashBytes.Serialize(s);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata16(s);
        hashBytes.Unserialize(s);
    }

    CAddressIndexIteratorKey(CChainParams::Base58Type addressType, uint160 addressHash) {
        type = addressType;
        hashBytes = addressHash;
    }

    CAddressIndexIteratorKey(int16_t addressType, uint160 addressHash) {
        type = addressType;
        hashBytes = addressHash;
    }

    CAddressIndexIteratorKey() {
        SetNull();
    }

    void SetNull() {
        type = CChainParams::Base58Type::UNKNOWN_TYPE;
        hashBytes.SetNull();
    }
};


struct CAddressDelegationIndexIteratorKey {
	uint160 hashStaker;
	uint160 hashOwner;

	size_t GetSerializeSize(int nType, int nVersion) const {
		return 40;
	}
	template<typename Stream>
	void Serialize(Stream& s) const {
		hashStaker.Serialize(s);
		hashOwner.Serialize(s);
	}
	template<typename Stream>
	void Unserialize(Stream& s) {
		hashStaker.Unserialize(s);
		hashOwner.Unserialize(s);
	}

	CAddressDelegationIndexIteratorKey(uint160 addressStaker, uint160 addressOwner) {
		hashStaker = addressStaker;
		hashOwner = addressOwner;
	}

	CAddressDelegationIndexIteratorKey() {
		SetNull();
	}

	void SetNull() {
		hashStaker.SetNull();
		hashOwner.SetNull();
	}
};


struct CAddressIndexIteratorHeightKey {
    int16_t type;
    uint160 hashBytes;
    int blockHeight;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 25;
    }
    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata16(s, type);
        hashBytes.Serialize(s);
        ser_writedata32be(s, blockHeight);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata16(s);
        hashBytes.Unserialize(s);
        blockHeight = ser_readdata32be(s);
    }

    CAddressIndexIteratorHeightKey(CChainParams::Base58Type addressType, uint160 addressHash, int height) {
        type = addressType;
        hashBytes = addressHash;
        blockHeight = height;
    }

    CAddressIndexIteratorHeightKey() {
        SetNull();
    }

    void SetNull() {
        type = CChainParams::Base58Type::UNKNOWN_TYPE;
        hashBytes.SetNull();
        blockHeight = 0;
    }
};

struct CAddressOwnerIdentificator {
    int16_t type;
    uint160 hashBytes;
    int blockHeight;

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata16(s, type);
        hashBytes.Serialize(s);
        ser_writedata32be(s, blockHeight);
    }
    template<typename Stream>
    void Unserialize(Stream& s) {
        type = ser_readdata16(s);
        hashBytes.Unserialize(s);
        blockHeight = ser_readdata32be(s);
    }

    CAddressOwnerIdentificator(CChainParams::Base58Type addressType, uint160 addressHash, int nHeight) {
        type = addressType;
        hashBytes = addressHash;
        blockHeight = nHeight;
    }

    CAddressOwnerIdentificator() {
        SetNull();
    }

    void SetNull() {
        type = CChainParams::Base58Type::UNKNOWN_TYPE;
        hashBytes.SetNull();
        blockHeight = 0;
    }
};


struct CSupplyIndexKey {
    int blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(blockHeight);
    }

    CSupplyIndexKey(int height) {
        blockHeight = height;
    }

    CSupplyIndexKey() {
        SetNull();
    }

    void SetNull() {
        blockHeight = 0;
    }

    bool IsNull() const {
        return (blockHeight == 0);
    }
};


struct CSupplyIndexValue {
    CAmount satoshis;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(satoshis);
    }

    CSupplyIndexValue(CAmount sats) {
        satoshis = sats;
    }

    CSupplyIndexValue() {
        SetNull();
    }

    void SetNull() {
        satoshis = -1;
    }

    bool IsNull() const {
        return (satoshis == -1);
    }
};


#endif // BITCOIN_SPENTINDEX_H
