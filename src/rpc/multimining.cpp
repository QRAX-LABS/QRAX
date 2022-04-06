// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "clientversion.h"
#include "httpserver.h"
#include "init.h"
#include "multimining/multimining_programs.h"
#include "multimining/multimining.h"
#include "invalid.h"

#include "net.h"
#include "netbase.h"
#include "rpc/server.h"
#include "timedata.h"
#include "util.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>

#include <univalue.h>

extern std::vector<CSporkDef> sporkDefs;

/**
 * @note Do not add or change anything in the information returned by this
 * method. `getinfo` exists for backwards-compatibility only. It combines
 * information from wildly different sources in the program, which is a mess,
 * and is thus planned to be deprecated eventually.
 *
 * Based on the source of the information, new information should be added to:
 * - `getblockchaininfo`,
 * - `getnetworkinfo` or
 * - `getwalletinfo`
 *
 * Or alternatively, create a specific query method for the information.
 **/

UniValue getassettarifs(const JSONRPCRequest& request)
{
    if (request.params.size() == 1 && request.params[0].get_str() == "show") {
        UniValue ret(UniValue::VOBJ);
        for (const auto& programDef : mmProgramDefs) {
            UniValue program(UniValue::VOBJ);
            CMultiMiningProgramMessage activeProgram;
            if (mmProgramManager.GetProgram(programDef.programId, activeProgram)) {
                program.pushKV("status", activeProgram.nActive > 0 ? "Active" : "Inactive");
                program.pushKV(MIN_WALLET_BALANCE_FIELD, activeProgram.minWalletBalance / COIN);
                program.pushKV(MIN_STRUCTURE_BALANCE_FIELD, activeProgram.minStructureBalance / COIN);
                program.pushKV(DAILY_STANDALONE_PERCENT_FIELD, strprintf("%03d.%02d",  activeProgram.dailyPercentStandalone / 100, activeProgram.dailyPercentStandalone % 100));
                program.pushKV(DAILY_STRUCTURE_PERCENT_FIELD, strprintf("%03d.%02d",  activeProgram.dailyPercentStructure / 100, activeProgram.dailyPercentStructure % 100));
            } else {
                program.pushKV("status", programDef.active > 0 ? "Active" : "Inactive");
                program.pushKV(MIN_WALLET_BALANCE_FIELD, programDef.minWalletBalance / COIN);
                program.pushKV(MIN_STRUCTURE_BALANCE_FIELD, programDef.minStructureBalance / COIN);
                program.pushKV(DAILY_STANDALONE_PERCENT_FIELD, strprintf("%03d.%02d",  programDef.dailyPercentStandalone / 100, programDef.dailyPercentStandalone % 100));
                program.pushKV(DAILY_STRUCTURE_PERCENT_FIELD, strprintf("%03d.%02d",  programDef.dailyPercentStructure / 100, programDef.dailyPercentStructure % 100));
            }
            ret.pushKV(programDef.name, program);
        }
        return ret;
    } else if (request.params.size() == 3) {
        // advanced mode, update spork values
        multiminingProgramId nProgramId = mmProgramManager.GetProgramIDByName(request.params[0].get_str());
        if (nProgramId == MULTIMINING_PROGRAM_INVALID) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid program name");
        }

        std::string field = request.params[1].get_str();
        bool status = false;
        uint16_t percent = 0;
        int64_t balance = 0;

        if (field == MIN_WALLET_BALANCE_FIELD || field == MIN_STRUCTURE_BALANCE_FIELD) {
            balance = request.params[2].get_int64() * COIN;
        } else if (field == DAILY_STANDALONE_PERCENT_FIELD || field == DAILY_STRUCTURE_PERCENT_FIELD) {
            percent = request.params[2].get_real() * 100;
        } else if (field == "status") {
            status = request.params[2].get_int();
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid field name");
        }

        //broadcast new spork
        if (mmProgramManager.UpdateProgram(nProgramId, field, status, percent, balance)) {
            return "success";
        } else {
            return "failure";
        }
    }

    throw std::runtime_error(
        "getassettarifs \"name\" ( value )\n"
        "\nReturn spork values or their active state.\n"

        "\nArguments:\n"
        "1. \"name\"        (string, required)  \"show\" to show values, \"active\" to show active state.\n"
        "                       When set up as a spork signer, the name of the spork can be used to update it's value.\n"
        "2. value           (numeric, required when updating a spork) The new value for the spork.\n"

        "\nResult (show):\n"
        "{\n"
        "  \"spork_name\": nnn      (key/value) Key is the spork name, value is it's current value.\n"
        "  ,...\n"
        "}\n"

        "\nResult (active):\n"
        "{\n"
        "  \"spork_name\": true|false      (key/value) Key is the spork name, value is a boolean for it's active state.\n"
        "  ,...\n"
        "}\n"

        "\nResult (name):\n"
        " \"success|failure\"       (string) Whether or not the update succeeded.\n"

        "\nExamples:\n" +
        HelpExampleCli("getassettarifs", "show") + HelpExampleRpc("getassettarifs", "show"));
}

UniValue getassetstructure(const JSONRPCRequest& request)
{

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "getassetstructure\n"
            "\nReturns an object containing various state info.\n"
            "\nExamples:\n" +
            HelpExampleCli("getassetstructure", "") + HelpExampleRpc("getassetstructure", ""));

    if (!fMultiMiningIsInit) throw JSONRPCError(RPC_MISC_ERROR, "Multimining currently not available");

    CKeyID hash;
    CKeyID parentHash;

	UniValue ret(UniValue::VOBJ);
	UniValue childs(UniValue::VOBJ);
	UniValue delegations(UniValue::VARR);


    if (request.params.size() > 0) {
        hash.SetHex(request.params[0].get_str());
        parentHash = multiMiningManager.GetNodeParentKey(hash);
	} else if (!fMultiWalletsNode) {
        hash = multiMiningManager.getKeyId();
        parentHash = multiMiningManager.GetNodeParentKey(hash);
	} else if (request.params.size() == 0) {
		hash = multiMiningManager.getGenesisKeyId();
	}

	if (hash.IsNull() || parentHash.IsNull()) throw JSONRPCError(RPC_MISC_ERROR, "Must be set hash");

	if (invalid_out::ContainsAssetKey(hash)) throw JSONRPCError(RPC_MISC_ERROR, "Hash not found");

	bool isGenesisHash = false;
    if (hash == multiMiningManager.getGenesisKeyId()) isGenesisHash = true;

    ret.pushKV("walletIdentHash", hash.GetHex());
    ret.pushKV("parentWalletIdentHash", parentHash.GetHex());
    double balance = 0;
    double structureBalance = 0;
    double currentAssets = 0;
    std::string programName = "0.00";
    double timePeriod = 0;
    double currentPercent = 0;
    uint64_t lastPaymentTime = 0;
    CAssetNode *node = multiMiningManager.GetNode(hash);
    uint8_t start = 0;
    if (node != nullptr && !node->getKeyId().IsNull())
    {
        balance = node->totalBalance;
        structureBalance = multiMiningTree.GetStructureNodeBalance(node, start);
        lastPaymentTime = node->GetUpdateTime();
        timePeriod = GetAdjustedTime() - lastPaymentTime;
        currentPercent = multiMiningManager.GetOwnPercentProgram(hash);
        double asset = balance * currentPercent * timePeriod / 864000000;
        currentAssets = asset + node->profitBeforePercentDowngrade;
        programName = mmProgramManager.GetProgramNameByID(node->currentProgramId);
    }


    if (isGenesisHash) structureBalance = multiMiningTree.GetStructureNodeBalance(hash, start);

    ret.pushKV("walletBalance", FormatMoney(balance));
    ret.pushKV("structureBalance", FormatMoney(structureBalance));
    ret.pushKV("currentAssets", FormatMoney(currentAssets));
    ret.pushKV("percent", currentPercent / 100.0);
    ret.pushKV("lastPaymentTime", lastPaymentTime);

    std::vector<CAssetNode*> vNodes = multiMiningTree.GetNodeStructure(hash);
    if (vNodes.size() > 0) {
        for (CAssetNode* cnode : vNodes)
        {
            UniValue child(UniValue::VOBJ);
            child.pushKV("walletBalance", FormatMoney(cnode->totalBalance));
            child.pushKV("structureBalance", FormatMoney(multiMiningTree.GetStructureNodeBalance(cnode, 0)));
            lastPaymentTime = cnode->GetUpdateTime();
            timePeriod = GetTime() - lastPaymentTime;
            currentPercent = multiMiningManager.GetOwnPercentProgram(cnode->getKeyId());
            double asset = cnode->totalBalance * currentPercent * timePeriod / 864000000;
            currentAssets = asset + cnode->profitBeforePercentDowngrade;
            child.pushKV("currentAssets", FormatMoney(currentAssets));
            child.pushKV("percent", currentPercent / 100.0);
            child.pushKV("lastPaymentTime", lastPaymentTime);
            childs.pushKV(cnode->getKeyId().GetHex(), child);
        }
    }

    ret.pushKV("childs", childs);

	if (node->delegations.size() > 0) {
		for (CAssetDelegation* d : node->delegations)
		{
			if (d->isActivated() == false || d->getAmount() == 0 || d->getPercent() == 0) continue;
			UniValue delegation(UniValue::VOBJ);
			CAddressOwnerIdentificator owner;
			if (!GetAddressIdentification(d->getOwnerKey(), CChainParams::PUBKEY_ADDRESS, owner)) continue;

			delegation.pushKV("staker", d->getStakerKey().GetHex());
			delegation.pushKV("delegator", d->getOwnerKey().GetHex());
			delegation.pushKV("owner", owner.hashBytes.GetHex());
			delegation.pushKV("amount", FormatMoney(d->getAmount()));
			delegation.pushKV("timestamp", d->GetUpdateTime());
			double currentPercent = d->getPercent();
			double currentBalance = d->getAmount();
			double timePeriod = GetAdjustedTime() - d->GetUpdateTime();
			double total = (currentBalance * currentPercent * timePeriod / 864000000) + d->getNHalfProfit();
			delegation.pushKV("profit", FormatMoney(total));
			delegation.pushKV("percent", d->getPercent() / 100);

			delegations.push_back(delegation);
		}
	}

	ret.pushKV("delegations", delegations);

    return ret;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "multimining",            "getassettarifs",             &getassettarifs,             true  },
    { "multimining",            "getassetstructure",         &getassetstructure,         true  },
    /* Not shown in help */
};

void RegisterMultiMiningRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
