// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "core_io.h"
#include "init.h"
#include "keystore.h"
#include "validationinterface.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/policy.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/sign.h"
#include "script/standard.h"
#include "uint256.h"
#include "utilmoneystr.h"
#include "spentindex.h"
#include "validation.h"
#ifdef ENABLE_WALLET
#include "sapling/address.h"
#include "sapling/key_io_sapling.h"
#include "wallet/wallet.h"
#endif

#include <future>
#include <stdint.h>

#include <univalue.h>

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry)
{
    // Call into TxToUniv() in bitcoin-common to decode the transaction hex.
    //
    // Blockchain contextual information (confirmations and blocktime) is not
    // available to code in bitcoin-common, so we query them here and push the
    // data into the returned UniValue.
    TxToUniv(tx, uint256(), entry);

    // Sapling
    if (pwalletMain && tx.IsShieldedTx()) {
        // Add information that only this wallet knows about the transaction if is possible
        if (pwalletMain->HasSaplingSPKM()) {
            std::vector<libzcash::SaplingPaymentAddress> addresses =
                    pwalletMain->GetSaplingScriptPubKeyMan()->FindMySaplingAddresses(tx);
            UniValue addrs(UniValue::VARR);
            for (const auto& addr : addresses) {
                addrs.push_back(KeyIO::EncodePaymentAddress(addr));
            }
            entry.pushKV("shielded_addresses", addrs);
        }
    }

    if (!hashBlock.IsNull()) {
        entry.pushKV("blockhash", hashBlock.GetHex());
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                entry.pushKV("confirmations", 1 + chainActive.Height() - pindex->nHeight);
                entry.pushKV("time", pindex->GetBlockTime());
                entry.pushKV("blocktime", pindex->GetBlockTime());
            }
            else
                entry.pushKV("confirmations", 0);
        }
    }
}

std::string GetSaplingTxHelpInfo()
{
    return "  \"valueBalance\": n,          (numeric) The net value of spend transfers minus output transfers\n"
           "  \"valueBalanceSat\": n,       (numeric) `valueBalance` in sats\n"
           "  \"vShieldSpend\": [               (array of json objects)\n"
           "     {\n"
           "       \"cv\": \"hex\",         (string) A value commitment to the value of the input note\n"
           "       \"anchor\": hex,         (string) A Merkle root of the Sapling note commitment tree at some block height in the past\n"
           "       \"nullifier\": hex,       (string) The nullifier of the input note\n"
           "       \"rk\": hex,              (string) The randomized public key for spendAuthSig\n"
           "       \"proof\": hex,           (string) A zero-knowledge proof using the spend circuit\n"
           "       \"spendAuthSig\": hex,    (string) A signature authorizing this spend\n"
           "     }\n"
           "     ,...\n"
           "  ],\n"
           "  \"vShieldOutput\": [             (array of json objects)\n"
           "     {\n"
           "       \"cv\": hex,                  (string) A value commitment to the value of the output note\n"
           "       \"cmu\": hex,                 (string) The u-coordinate of the note commitment for the output note\n"
           "       \"ephemeralKey\": hex,         (string) A Jubjub public key\n"
           "       \"encCiphertext\": hex,       (string) A ciphertext component for the encrypted output note\n"
           "       \"outCiphertext\": hex,       (string) A ciphertext component for the encrypted output note\n"
           "       \"proof\": hex,               (string) A zero-knowledge proof using the output circuit\n"
           "     }\n"
           "     ,...\n"
           "  ],\n"
           "  \"bindingSig\": hex,       (string) Prove consistency of valueBalance with the value commitments in spend descriptions and output descriptions, and proves knowledge of the randomness used for the spend and output value commitments\n";
}

UniValue getrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "getrawtransaction \"txid\" ( verbose \"blockhash\" )\n"

            "\nNOTE: By default this function only works sometimes. This is when the tx is in the mempool\n"
            "or there is an unspent output in the utxo for this transaction. To make it always work,\n"
            "you need to maintain a transaction index, using the -txindex command line option.\n"

            "\nReturn the raw transaction data.\n"
            "\nIf verbose is 'true', returns an Object with information about 'txid'.\n"
            "If verbose is 'false' or omitted, returns a string that is serialized, hex-encoded data for 'txid'.\n"

            "\nArguments:\n"
            "1. \"txid\"      (string, required) The transaction id\n"
            "2. verbose     (bool, optional, default=false) If false, return a string, otherwise return a json object\n"
            "3. \"blockhash\" (string, optional) The block in which to look for the transaction\n"

            "\nResult (if verbose is not set or set to false):\n"
            "\"data\"      (string) The serialized, hex-encoded data for 'txid'\n"

            "\nResult (if verbose is set to true):\n"
            "{\n"
            "  \"in_active_chain\": b, (bool) Whether specified block is in the active chain or not (only present with explicit \"blockhash\" argument)\n"
            "  \"hex\" : \"data\",       (string) The serialized, hex-encoded data for 'txid'\n"
            "  \"txid\" : \"id\",        (string) The transaction id (same as provided)\n"
            "  \"size\" : n,             (numeric) The serialized transaction size\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"type\" : n,             (numeric) The type\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) \n"
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"sequence\": n      (numeric) The script sequence number\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [              (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in QRAX\n"
            "       \"n\" : n,                    (numeric) index\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"qraxaddress\"        (string) qrax address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            + GetSaplingTxHelpInfo() +
            //"  \"shielded_addresses\"      (json array of string) the shielded addresses involved in this transaction if possible (only for shielded transactions and the tx owner/viewer)\n"
            //"  \"extraPayloadSize\" : n    (numeric) Size of extra payload. Only present if it's a special TX\n"
            //"  \"extraPayload\" : \"hex\"  (string) Hex encoded extra payload data. Only present if it's a special TX\n"
            "  \"blockhash\" : \"hash\",   (string) the block hash\n"
            "  \"confirmations\" : n,      (numeric) The confirmations\n"
            "  \"time\" : ttt,             (numeric) The transaction time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"blocktime\" : ttt         (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getrawtransaction", "\"mytxid\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" true")
            + HelpExampleRpc("getrawtransaction", "\"mytxid\", true")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" false \"myblockhash\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" true \"myblockhash\"")
        );

    LOCK(cs_main);

    bool in_active_chain = true;
    uint256 hash = ParseHashV(request.params[0], "parameter 1");
    CBlockIndex* blockindex = nullptr;

    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].isNum() ? (request.params[1].get_int() != 0) : request.params[1].get_bool();
    }

    if (!request.params[2].isNull()) {
        uint256 blockhash = ParseHashV(request.params[2], "parameter 3");
        BlockMap::iterator it = mapBlockIndex.find(blockhash);
        if (it == mapBlockIndex.end()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found");
        }
        blockindex = it->second;
        in_active_chain = chainActive.Contains(blockindex);
    }

    CTransactionRef tx;
    uint256 hash_block;
    if (!GetTransaction(hash, tx, hash_block, true, blockindex)) {
        std::string errmsg;
        if (blockindex) {
            if (!(blockindex->nStatus & BLOCK_HAVE_DATA)) {
                throw JSONRPCError(RPC_MISC_ERROR, "Block not available");
            }
            errmsg = "No such transaction found in the provided block";
        } else {
            errmsg = fTxIndex
              ? "No such mempool or blockchain transaction"
              : "No such mempool transaction. Use -txindex to enable blockchain transaction queries";
        }
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errmsg + ". Use gettransaction for wallet transactions.");
    }

    if (!fVerbose) {
        return EncodeHexTx(*tx);
    }

    UniValue result(UniValue::VOBJ);
    if (blockindex) result.pushKV("in_active_chain", in_active_chain);
    TxToJSON(*tx, hash_block, result);
    return result;
}


UniValue createrawdelegation(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
		throw std::runtime_error(
			"createrawdelegation [{\"txid\":\"id\",\"vout\":n},...] {\"address\":amount,...} ( locktime ) ( walletIdent ) \n"
			"\nCreate a transaction spending the given inputs and sending to the given addresses.\n"
			"Returns hex-encoded raw transaction.\n"
			"Note that the transaction's inputs are not signed, and\n"
			"it is not stored in the wallet or transmitted to the network.\n"

			"\nArguments:\n"
			"1. \"transactions\"        (string, required) A json array of json objects\n"
			"     [\n"
			"       {\n"
			"         \"txid\":\"id\",  (string, required) The transaction id\n"
			"         \"vout\":n,       (numeric, required) The output number\n"
			"         \"sequence\":n    (numeric, optional) The sequence number\n"
			"       }\n"
			"       ,...\n"
			"     ]\n"
			"2. \"addresses\"           (string, required) a json object with scripts as keys and amounts as values\n"
			"    {\n"
			"      \"script\": x.xxx   (numeric, required) The key is the qrax script, the value is the qrax amount\n"
			"      ,...\n"
			"    }\n"
			"3. locktime                (numeric, optional, default=0) Raw locktime. Non-0 value also locktime-activates inputs\n"
			"4. walletIdent             (string, optional, default=currentWalletIdent) Used only when -multiwallets is set\n"

			"\nResult:\n"
			"\"transaction\"            (string) hex string of the transaction\n"

			"\nExamples\n" +
			HelpExampleCli("createrawdelegation", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"") + HelpExampleRpc("createrawdelegation", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\""));

	LOCK(cs_main);
	RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VOBJ, UniValue::VNUM});
	if (request.params[0].isNull() || request.params[1].isNull())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

	UniValue inputs = request.params[0].get_array();

	UniValue sendTo = request.params[1].get_obj();

	const UniValue& staker = find_value(sendTo, "staker");
	const UniValue& owner = find_value(sendTo, "owner");
	const UniValue& amount = find_value(sendTo, "amount");

	CMutableTransaction rawTx;

	if (request.params.size() > 2 && !request.params[3].isNull()) {
		int64_t nLockTime = request.params[3].get_int64();
		if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
		rawTx.nLockTime = nLockTime;
	}

	EnsureWalletIsUnlocked();

	CAmount nTxAmount = AmountFromValue(amount);
	CAmount nFeeNeeded = ::minRelayTxFee.GetPercentFee(nTxAmount);

	CAmount amountVin = 0;
	for (unsigned int idx = 0; idx < inputs.size(); idx++) {
		const UniValue& input = inputs[idx];
		const UniValue& o = input.get_obj();

		uint256 txid = ParseHashO(o, "txid");

		const UniValue& vout_v = find_value(o, "vout");
		if (!vout_v.isNum())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
		int nOutput = vout_v.get_int();
		if (nOutput < 0)
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

		uint32_t nSequence = (rawTx.nLockTime ? std::numeric_limits<uint32_t>::max() - 1 : std::numeric_limits<uint32_t>::max());

		// set the sequence number if passed in the parameters object
		const UniValue& sequenceObj = find_value(o, "sequence");
		if (sequenceObj.isNum()) {
			int64_t seqNr64 = sequenceObj.get_int64();
			if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max())
				throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
			else
				nSequence = (uint32_t)seqNr64;
		}

		CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);
		rawTx.vin.push_back(in);

		std::map<uint256, CWalletTx>::const_iterator it = pwalletMain->mapWallet.find(txid);
		if (it != pwalletMain->mapWallet.end()) {
			const CWalletTx* pcoin = &it->second;

			amountVin += pcoin->tx->vout[nOutput].nValue;
		}
		if (amountVin >= (nTxAmount + nFeeNeeded))
			break;
	}

	std::string genesisKey = Params().GetConsensus().strGenesisPubKeyHex;
	std::vector<unsigned char> data(ParseHex(genesisKey));
	CPubKey pubKey(data.begin(), data.end());

	if (amountVin < AmountFromValue(amount))
		throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, std::string("Incuffient funds"));

	bool isStaking = false;
	CTxDestination ownerAddr = DecodeDestination(owner.get_str(), isStaking);
	if (!IsValidDestination(ownerAddr) || isStaking)
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid owner QRAX address: ")+owner.get_str());


	isStaking = false;
	CTxDestination stakeAddr = DecodeDestination(staker.get_str(), isStaking);
	if (!IsValidDestination(stakeAddr) || !isStaking)
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid QRAX staking address");


	CKeyID* ownerKey = boost::get<CKeyID>(&ownerAddr);
	CKeyID* stakerKey = boost::get<CKeyID>(&stakeAddr);

	CScript scriptPubKey = GetScriptForStakeDelegation(*stakerKey, *ownerKey);

	CTxOut out(nTxAmount, scriptPubKey);
	rawTx.vout.push_back(out);

	if (amountVin > (nTxAmount + nFeeNeeded))
	{
		CScript scriptPubKey = GetScriptForDestination(ownerAddr);
		CTxOut out(amountVin - (nTxAmount + nFeeNeeded), scriptPubKey);
		rawTx.vout.push_back(out);
	}

	if (!fMultiWalletsNode && pwalletMain) {
		rawTx.walletSenderId = pwalletMain->GetWalletIdentHash();
	}

	if (fMultiWalletsNode && pwalletMain) {
		CAddressOwnerIdentificator ident;
		if (GetAddressIdentification(*ownerKey, CChainParams::PUBKEY_ADDRESS, ident))
		{
			rawTx.walletSenderId = CKeyID(ident.hashBytes);
		}
	}

	return EncodeHexTx(rawTx);
}


UniValue createrawtransaction(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
			"createrawtransaction [{\"txid\":\"id\",\"vout\":n},...] {\"address\":amount,...} ( locktime ) ( walletIdent ) \n"
            "\nCreate a transaction spending the given inputs and sending to the given addresses.\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.\n"

            "\nArguments:\n"
            "1. \"transactions\"        (string, required) A json array of json objects\n"
            "     [\n"
            "       {\n"
            "         \"txid\":\"id\",  (string, required) The transaction id\n"
            "         \"vout\":n,       (numeric, required) The output number\n"
            "         \"sequence\":n    (numeric, optional) The sequence number\n"
            "       }\n"
            "       ,...\n"
            "     ]\n"
            "2. \"addresses\"           (string, required) a json object with addresses as keys and amounts as values\n"
            "    {\n"
            "      \"address\": x.xxx   (numeric, required) The key is the qrax address, the value is the qrax amount\n"
            "      ,...\n"
            "    }\n"
            "3. locktime                (numeric, optional, default=0) Raw locktime. Non-0 value also locktime-activates inputs\n"
			"4. walletIdent             (string, optional, default=currentWalletIdent) Used only when -multiwallets is set\n"

            "\nResult:\n"
            "\"transaction\"            (string) hex string of the transaction\n"

            "\nExamples\n" +
            HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"") + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\""));

    LOCK(cs_main);
    RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VOBJ, UniValue::VNUM});
    if (request.params[0].isNull() || request.params[1].isNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = request.params[0].get_array();
    UniValue sendTo = request.params[1].get_obj();

    CMutableTransaction rawTx;

    if (request.params.size() > 2 && !request.params[2].isNull()) {
        int64_t nLockTime = request.params[2].get_int64();
        if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        rawTx.nLockTime = nLockTime;
    }

    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t nSequence = (rawTx.nLockTime ? std::numeric_limits<uint32_t>::max() - 1 : std::numeric_limits<uint32_t>::max());

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            else
                nSequence = (uint32_t)seqNr64;
        }

        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
    }


    std::string genesisKey = Params().GetConsensus().strGenesisPubKeyHex;
    std::vector<unsigned char> data(ParseHex(genesisKey));
    CPubKey pubKey(data.begin(), data.end());

    std::set<CTxDestination> setAddress;
    std::vector<std::string> addrList = sendTo.getKeys();
    for (const std::string& name_ : addrList) {
        CTxDestination address = DecodeDestination(name_);
        if (!IsValidDestination(address))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid QRAX address: ")+name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ")+name_);
        setAddress.insert(address);

        CScript scriptPubKey = GetScriptForDestination(address);
        //
        CAmount nTxAmount = AmountFromValue(sendTo[name_]);
        uint160 curhash;
        if (scriptPubKey.IsPayToScriptHash()) {
            std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
            curhash = uint160(hashBytes);
        } else if (scriptPubKey.IsPayToPublicKeyHash()) {
            std::vector<unsigned char> hashBytes(scriptPubKey.begin()+3, scriptPubKey.begin()+23);
            curhash = uint160(hashBytes);
        } else if (scriptPubKey.IsPayToPublicKey()) {
            curhash = uint160(Hash160(scriptPubKey.begin()+1, scriptPubKey.end()-1));
        }
        if (curhash == pubKey.GetID()) {
            nTxAmount = 0;
            rawTx.fBurnAmountTransaction = true;
        }
        //
        CTxOut out(nTxAmount, scriptPubKey);
        rawTx.vout.push_back(out);
    }
	CKeyID walletIdent = CKeyID();

	if (request.params.size() == 4) {
		walletIdent.SetHex(request.params[3].get_str());
	}

    if (!fMultiWalletsNode && pwalletMain) {
        rawTx.walletSenderId = pwalletMain->GetWalletIdentHash();
	} else
		rawTx.walletSenderId = walletIdent;

    return EncodeHexTx(rawTx);
}

UniValue decoderawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decoderawtransaction \"hexstring\"\n"
            "\nReturn a JSON object representing the serialized, hex-encoded transaction.\n"

            "\nArguments:\n"
            "1. \"hex\"      (string, required) The transaction hex string\n"

            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"id\",        (string) The transaction id\n"
            "  \"size\" : n,             (numeric) The transaction size\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"type\" : n,             (numeric) The type\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) The output number\n"
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"sequence\": n     (numeric) The script sequence number\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [             (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in QRAX\n"
            "       \"n\" : n,                    (numeric) index\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"12tvKAXCxZjSmdNbao16dKXC8tRWfcF5oc\"   (string) qrax address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            + GetSaplingTxHelpInfo() +
            "  \"extraPayloadSize\" : n    (numeric) Size of extra payload. Only present if it's a special TX\n"
            "  \"extraPayload\" : \"hex\"  (string) Hex encoded extra payload data. Only present if it's a special TX\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("decoderawtransaction", "\"hexstring\"") + HelpExampleRpc("decoderawtransaction", "\"hexstring\""));

    LOCK(cs_main);
    RPCTypeCheck(request.params, {UniValue::VSTR});

    CMutableTransaction mtx;

    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    UniValue result(UniValue::VOBJ);
    TxToJSON(CTransaction(std::move(mtx)), UINT256_ZERO, result);

    return result;
}

UniValue decodescript(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decodescript \"hex\"\n"
            "\nDecode a hex-encoded script.\n"

            "\nArguments:\n"
            "1. \"hex\"     (string) the hex encoded script\n"

            "\nResult:\n"
            "{\n"
            "  \"asm\":\"asm\",   (string) Script public key\n"
            "  \"hex\":\"hex\",   (string) hex encoded public key\n"
            "  \"type\":\"type\", (string) The output type\n"
            "  \"reqSigs\": n,    (numeric) The required signatures\n"
            "  \"addresses\": [   (json array of string)\n"
            "     \"address\"     (string) qrax address\n"
            "     ,...\n"
            "  ],\n"
            "  \"p2sh\",\"address\" (string) script address\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("decodescript", "\"hexstring\"") + HelpExampleRpc("decodescript", "\"hexstring\""));

    LOCK(cs_main);
    RPCTypeCheck(request.params, {UniValue::VSTR});

    UniValue r(UniValue::VOBJ);
    CScript script;
    if (request.params[0].get_str().size() > 0) {
        std::vector<unsigned char> scriptData(ParseHexV(request.params[0], "argument"));
        script = CScript(scriptData.begin(), scriptData.end());
    } else {
        // Empty scripts are valid
    }
    ScriptPubKeyToUniv(script, r, false);

    r.pushKV("p2sh", EncodeDestination(CScriptID(script)));
    return r;
}

/** Pushes a JSON object for script verification or signing errors to vErrorsRet. */
static void TxInErrorToJSON(const CTxIn& txin, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.pushKV("txid", txin.prevout.hash.ToString());
    entry.pushKV("vout", (uint64_t)txin.prevout.n);
    entry.pushKV("scriptSig", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
    entry.pushKV("sequence", (uint64_t)txin.nSequence);
    entry.pushKV("error", strMessage);
    vErrorsRet.push_back(entry);
}

UniValue fundrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "fundrawtransaction \"hexstring\" ( options )\n"
            "\nAdd inputs to a transaction until it has enough in value to meet its out value.\n"
            "This will not modify existing inputs, and will add one change output to the outputs.\n"
            "Note that inputs which were signed may need to be resigned after completion since in/outputs have been added.\n"
            "The inputs added will not be signed, use signrawtransaction for that.\n"
            "Note that all existing inputs must have their previous output transaction be in the wallet.\n"
            "Note that all inputs selected must be of standard form and P2SH scripts must be "
            "in the wallet using importaddress or addmultisigaddress (to calculate fees).\n"
            "You can see whether this is the case by checking the \"solvable\" field in the listunspent output.\n"
            "Only pay-to-pubkey, multisig, and P2SH versions thereof are currently supported for watch-only\n"

            "\nArguments:\n"
            "1. \"hexstring\"    (string, required) The hex string of the raw transaction\n"
            "2. options          (object, optional)\n"
            "   {\n"
            "     \"changeAddress\"     (string, optional, default pool address) The QRAX address to receive the change\n"
            "     \"changePosition\"    (numeric, optional, default random) The index of the change output\n"
            "     \"includeWatching\"   (boolean, optional, default false) Also select inputs which are watch only\n"
            "     \"lockUnspents\"      (boolean, optional, default false) Lock selected unspent outputs\n"
                " \"feeRate\"           (numeric, optional, default 0=estimate) Set a specific feerate (QRAX per KB)\n"
            "   }\n"
            "\nResult:\n"
            "{\n"
            "  \"hex\":       \"value\", (string)  The resulting raw transaction (hex-encoded string)\n"
            "  \"fee\":       n,         (numeric) The fee added to the transaction\n"
            "  \"changepos\": n          (numeric) The position of the added change output, or -1\n"
            "}\n"
            "\"hex\"             \n"
            "\nExamples:\n"
            "\nCreate a transaction with no inputs\n"
            + HelpExampleCli("createrawtransaction", "\"[]\" \"{\\\"myaddress\\\":0.01}\"") +
            "\nAdd sufficient unsigned inputs to meet the output value\n"
            + HelpExampleCli("fundrawtransaction", "\"rawtransactionhex\"") +
            "\nSign the transaction\n"
            + HelpExampleCli("signrawtransaction", "\"fundedtransactionhex\"") +
            "\nSend the transaction\n"
            + HelpExampleCli("sendrawtransaction", "\"signedtransactionhex\"")
            );

    if (!pwalletMain)
        throw std::runtime_error("wallet not initialized");

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwalletMain->BlockUntilSyncedToCurrentChain();

    RPCTypeCheck(request.params, {UniValue::VSTR});

    CTxDestination changeAddress = CNoDestination();
    int changePosition = -1;
    bool includeWatching = false;
    bool lockUnspents = false;
    CFeeRate feeRate = CFeeRate(0);
    bool overrideEstimatedFeerate = false;

    if (request.params.size() > 1) {
        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ});
        UniValue options = request.params[1];
        RPCTypeCheckObj(options,
            {
                {"changeAddress", UniValueType(UniValue::VSTR)},
                {"changePosition", UniValueType(UniValue::VNUM)},
                {"includeWatching", UniValueType(UniValue::VBOOL)},
                {"lockUnspents", UniValueType(UniValue::VBOOL)},
                {"feeRate", UniValueType()}, // will be checked below
            },
            true, true);

        if (options.exists("changeAddress")) {
            changeAddress = DecodeDestination(options["changeAddress"].get_str());

            if (!IsValidDestination(changeAddress))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "changeAddress must be a valid QRAX address");
        }

        if (options.exists("changePosition"))
            changePosition = options["changePosition"].get_int();

        if (options.exists("includeWatching"))
            includeWatching = options["includeWatching"].get_bool();

        if (options.exists("lockUnspents"))
            lockUnspents = options["lockUnspents"].get_bool();

        if (options.exists("feeRate")) {
            feeRate = CFeeRate(AmountFromValue(options["feeRate"]));
            overrideEstimatedFeerate = true;
        }
    }

    // parse hex string from parameter
    CMutableTransaction origTx;
    if (!DecodeHexTx(origTx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    if (origTx.vout.size() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "TX must have at least one output");

    if (changePosition != -1 && (changePosition < 0 || (unsigned int) changePosition > origTx.vout.size()))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "changePosition out of bounds");

    CMutableTransaction tx(origTx);
    CAmount nFeeOut;
    std::string strFailReason;
    if(!pwalletMain->FundTransaction(tx, nFeeOut, overrideEstimatedFeerate, feeRate, changePosition, strFailReason, includeWatching, lockUnspents, changeAddress))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strFailReason);

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(tx));
    result.pushKV("changepos", changePosition);
    result.pushKV("fee", ValueFromAmount(nFeeOut));

    return result;
}

UniValue signrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 4)
        throw std::runtime_error(
            "signrawtransaction \"hexstring\" ( [{\"txid\":\"id\",\"vout\":n,\"scriptPubKey\":\"hex\",\"redeemScript\":\"hex\"},...] [\"privatekey1\",...] sighashtype )\n"
            "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
            "The second optional argument (may be null) is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the block chain.\n"
            "The third optional argument (may be null) is an array of base58-encoded private\n"
            "keys that, if given, will be the only keys used to sign the transaction.\n"
#ifdef ENABLE_WALLET
            + HelpRequiringPassphrase() + "\n"
#endif

            "\nArguments:\n"
            "1. \"hexstring\"     (string, required) The transaction hex string\n"
            "2. \"prevtxs\"       (string, optional) An json array of previous dependent transaction outputs\n"
            "     [               (json array of json objects, or 'null' if none provided)\n"
            "       {\n"
            "         \"txid\":\"id\",             (string, required) The transaction id\n"
            "         \"vout\":n,                  (numeric, required) The output number\n"
            "         \"scriptPubKey\": \"hex\",   (string, required) script key\n"
            "         \"redeemScript\": \"hex\"    (string, required for P2SH) redeem script\n"
            "         \"amount\": value            (numeric, required) The amount spent\n"
            "       }\n"
            "       ,...\n"
            "    ]\n"
            "3. \"privatekeys\"     (string, optional) A json array of base58-encoded private keys for signing\n"
            "    [                  (json array of strings, or 'null' if none provided)\n"
            "      \"privatekey\"   (string) private key in base58-encoding\n"
            "      ,...\n"
            "    ]\n"
            "4. \"sighashtype\"     (string, optional, default=ALL) The signature hash type. Must be one of\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"

            "\nResult:\n"
            "{\n"
            "  \"hex\" : \"value\",           (string) The hex-encoded raw transaction with signature(s)\n"
            "  \"complete\" : true|false,   (boolean) If the transaction has a complete set of signatures\n"
            "  \"errors\" : [                 (json array of objects) Script verification errors (if there are any)\n"
            "    {\n"
            "      \"txid\" : \"hash\",           (string) The hash of the referenced, previous transaction\n"
            "      \"vout\" : n,                (numeric) The index of the output to spent and used as input\n"
            "      \"scriptSig\" : \"hex\",       (string) The hex-encoded signature script\n"
            "      \"sequence\" : n,            (numeric) Script sequence number\n"
            "      \"error\" : \"text\"           (string) Verification or signing error related to the input\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("signrawtransaction", "\"myhex\"") + HelpExampleRpc("signrawtransaction", "\"myhex\""));

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwalletMain ? &pwalletMain->cs_wallet : NULL);
#else
    LOCK(cs_main);
#endif
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VARR, UniValue::VSTR}, true);

    std::vector<unsigned char> txData(ParseHexV(request.params[0], "argument 1"));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    std::vector<CMutableTransaction> txVariants;
    while (!ssData.empty()) {
        try {
            CMutableTransaction tx;
            ssData >> tx;
            txVariants.push_back(tx);
        } catch (const std::exception&) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
        }
    }

    if (txVariants.empty())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transaction");

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the rawtx:
    CMutableTransaction mergedTx(txVariants[0]);

    // Fetch previous transactions (inputs):
    std::map<COutPoint, std::pair<CScript, CAmount>> mapPrevOut; // todo: check why do we have this for regtest..
    if (Params().IsRegTestNet()) {
        for (const CTxIn &txbase : mergedTx.vin)
        {
            CTransactionRef tempTx;
            uint256 hashBlock;
            if (GetTransaction(txbase.prevout.hash, tempTx, hashBlock, true)) {
                // Copy results into mapPrevOut:
                mapPrevOut[txbase.prevout] = std::make_pair(tempTx->vout[txbase.prevout.n].scriptPubKey, tempTx->vout[txbase.prevout.n].nValue);
            }
        }
    }
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache& viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mergedTx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    bool fGivenKeys = false;
    CBasicKeyStore tempKeystore;
    if (request.params.size() > 2 && !request.params[2].isNull()) {
        fGivenKeys = true;
        UniValue keys = request.params[2].get_array();
        for (unsigned int idx = 0; idx < keys.size(); idx++) {
            UniValue k = keys[idx];
            CKey key = DecodeSecret(k.get_str());
            if (!key.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            tempKeystore.AddKey(key);
        }
    }
#ifdef ENABLE_WALLET
    else if (pwalletMain)
        EnsureWalletIsUnlocked();
#endif

    // Add previous txouts given in the RPC call:
    if (request.params.size() > 1 && !request.params[1].isNull()) {
        UniValue prevTxs = request.params[1].get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); idx++) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject())
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
                const Coin& coin = view.AccessCoin(out);
                if (!coin.IsSpent() && coin.out.scriptPubKey != scriptPubKey) {
                    std::string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coin.out.scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
                }

                Coin newcoin;
                newcoin.out.scriptPubKey = scriptPubKey;
                newcoin.out.nValue = 0;
                newcoin.nHeight = 1;
                if (prevOut.exists("amount")) {
                    newcoin.out.nValue = AmountFromValue(find_value(prevOut, "amount"));
                }
                view.AddCoin(out, std::move(newcoin), true);
            }

            // if redeemScript given and not using the local wallet (private keys
            // given), add redeemScript to the tempKeystore so it can be signed:
            if (fGivenKeys && scriptPubKey.IsPayToScriptHash()) {
                RPCTypeCheckObj(prevOut,
                    {
                        {"txid", UniValueType(UniValue::VSTR)},
                        {"vout", UniValueType(UniValue::VNUM)},
                        {"scriptPubKey", UniValueType(UniValue::VSTR)},
                        {"redeemScript", UniValueType(UniValue::VSTR)},
                    });
                UniValue v = find_value(prevOut, "redeemScript");
                if (!v.isNull()) {
                    std::vector<unsigned char> rsData(ParseHexV(v, "redeemScript"));
                    CScript redeemScript(rsData.begin(), rsData.end());
                    tempKeystore.AddCScript(redeemScript);
                }
            }
        }
    }

#ifdef ENABLE_WALLET
    const CKeyStore& keystore = ((fGivenKeys || !pwalletMain) ? tempKeystore : *pwalletMain);
#else
    const CKeyStore& keystore = tempKeystore;
#endif

    int nHashType = SIGHASH_ALL;
    if (request.params.size() > 3 && !request.params[3].isNull()) {
        static std::map<std::string, int> mapSigHashValues = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = request.params[3].get_str();
        if (mapSigHashValues.count(strHashType))
            nHashType = mapSigHashValues[strHashType];
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
    }

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mergedTx);
    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (Params().IsRegTestNet()) {
            if (mapPrevOut.count(txin.prevout) == 0 && coin.IsSpent())
            {
                TxInErrorToJSON(txin, vErrors, "Input not found");
                continue;
            }
        } else {
            if (coin.IsSpent()) {
                TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
                continue;
            }
        }

        const CScript& prevPubKey = (Params().IsRegTestNet() && mapPrevOut.count(txin.prevout) != 0 ? mapPrevOut[txin.prevout].first : coin.out.scriptPubKey);
        const CAmount& amount = (Params().IsRegTestNet() && mapPrevOut.count(txin.prevout) != 0 ? mapPrevOut[txin.prevout].second : coin.out.nValue);

        txin.scriptSig.clear();

        // if this is a P2CS script, select which key to use
        bool fColdStake = false;
        if (prevPubKey.IsPayToColdStaking()) {
            // if we have both keys, sign with the spender key
            fColdStake = !bool(IsMine(keystore, prevPubKey) & ISMINE_SPENDABLE_DELEGATED);
        }

        SignatureData sigdata;
        SigVersion sigversion = mergedTx.GetRequiredSigVersion();
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mergedTx.vout.size()))
            ProduceSignature(MutableTransactionSignatureCreator(&keystore, &mergedTx, i, amount, nHashType),
                    prevPubKey, sigdata, sigversion, fColdStake);

        // ... and merge in other signatures:
        for (const CMutableTransaction& txv : txVariants) {
            sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txConst, i, amount), sigdata, DataFromTransaction(txv, i));
        }

        UpdateTransaction(mergedTx, i, sigdata);

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, STANDARD_SCRIPT_VERIFY_FLAGS,
                TransactionSignatureChecker(&txConst, i, amount), sigversion, &serror)) {
            TxInErrorToJSON(txin, vErrors, ScriptErrorString(serror));
        }
    }
    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(mergedTx));
    result.pushKV("complete", fComplete);
    if (!vErrors.empty()) {
        result.pushKV("errors", vErrors);
    }

    return result;
}

UniValue sendrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "sendrawtransaction \"hexstring\" ( allowhighfees )\n"
            "\nSubmits raw transaction (serialized, hex-encoded) to local node and network.\n"
            "\nAlso see createrawtransaction and signrawtransaction calls.\n"

            "\nArguments:\n"
            "1. \"hexstring\"    (string, required) The hex string of the raw transaction)\n"
            "2. allowhighfees    (boolean, optional, default=false) Allow high fees\n"

            "\nResult:\n"
            "\"hex\"             (string) The transaction hash in hex\n"

            "\nExamples:\n"
            "\nCreate a transaction\n" +
            HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n" + HelpExampleCli("signrawtransaction", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n" + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a json rpc call\n" + HelpExampleRpc("sendrawtransaction", "\"signedhex\""));

    std::promise<void> promise;

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VBOOL});

    // parse hex string from parameter
    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    const uint256& hashTx = mtx.GetHash();

    bool fOverrideFees = false;
    if (request.params.size() > 1)
        fOverrideFees = request.params[1].get_bool();

    { // cs_main scope
    LOCK(cs_main);
    CCoinsViewCache& view = *pcoinsTip;
    bool fHaveChain = false;
    for (size_t o = 0; !fHaveChain && o < mtx.vout.size(); o++) {
        const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
        fHaveChain = !existingCoin.IsSpent();
    }
    bool fHaveMempool = mempool.exists(hashTx);
    if (!fHaveMempool && !fHaveChain) {
        CValidationState state;
        bool fMissingInputs;
        {
            if (!AcceptToMemoryPool(mempool, state, MakeTransactionRef(std::move(mtx)), true, &fMissingInputs, false, !fOverrideFees)) {
                if (state.IsInvalid()) {
                    throw JSONRPCError(RPC_TRANSACTION_REJECTED, strprintf("%i: %s", state.GetRejectCode(), state.GetRejectReason()));
                } else {
                    if (fMissingInputs) {
                        throw JSONRPCError(RPC_TRANSACTION_ERROR, "Missing inputs");
                    }
                    throw JSONRPCError(RPC_TRANSACTION_ERROR, state.GetRejectReason());
                }
            } else {
                // If wallet is enabled, ensure that the wallet has been made aware
                // of the new transaction prior to returning. This prevents a race
                // where a user might call sendrawtransaction with a transaction
                // to/from their wallet, immediately call some wallet RPC, and get
                // a stale result because callbacks have not yet been processed.
                CallFunctionInValidationInterfaceQueue([&promise] {
                    promise.set_value();
                });
            }
        }
    } else if (fHaveChain) {
        throw JSONRPCError(RPC_TRANSACTION_ALREADY_IN_CHAIN, "transaction already in block chain");
    }

    } // cs_main

    promise.get_future().wait();

    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    CInv inv(MSG_TX, hashTx);
    g_connman->ForEachNode([&inv](CNode* pnode)
    {
        pnode->PushInventory(inv);
    });

    return hashTx.GetHex();
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "rawtransactions",    "getrawtransaction",      &getrawtransaction,      true  },
    { "rawtransactions",    "createrawtransaction",   &createrawtransaction,   true  },
	{ "rawtransactions",    "createrawdelegation",    &createrawdelegation,    true  },
    { "rawtransactions",    "decoderawtransaction",   &decoderawtransaction,   true  },
    { "rawtransactions",    "decodescript",           &decodescript,           true  },
    { "rawtransactions",    "fundrawtransaction",     &fundrawtransaction,     false },
    { "rawtransactions",    "signrawtransaction",     &signrawtransaction,     false }, /* uses wallet if enabled */
    { "rawtransactions",    "sendrawtransaction",     &sendrawtransaction,     false },
};

void RegisterRawTransactionRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
