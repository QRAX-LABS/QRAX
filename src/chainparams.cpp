// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "chainparamsseeds.h"
#include "consensus/merkle.h"
#include "tinyformat.h"
#include "utilstrencodings.h"

#include <assert.h>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;
    txNew.walletSenderId = CKeyID();

    CBlock genesis;
    genesis.vtx.push_back(std::make_shared<const CTransaction>(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.nVersion = nVersion;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

void CChainParams::UpdateNetworkUpgradeParameters(Consensus::UpgradeIndex idx, int nActivationHeight)
{
    assert(IsRegTestNet()); // only available for regtest
    assert(idx > Consensus::BASE_NETWORK && idx < Consensus::MAX_NETWORK_UPGRADES);
    consensus.vUpgrades[idx].nActivationHeight = nActivationHeight;
}

/**
 * Build the genesis block. Note that the output of the genesis coinbase cannot
 * be spent as it did not originally exist in the database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, const std::string genesisPubKey)
{
    const char* pszTimestamp = "SoftBank-backed corporate lending startup was once valued at $4 billion and now leaves a trail of losses for investors and bank depositors";
    const CScript genesisOutputScript = CScript() << ParseHex(genesisPubKey) << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void generateGenesis(CBlock &genesis, const std::string &netname)
{
    return;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
static Checkpoints::MapCheckpoints mapCheckpoints = {
    {0, uint256S("00000e2864f744c05d9fc94c4f02f0810927ab2100abaae3f75e87f915dbcec1")},
    {30000, uint256S("00000001370b010ee69f255b9d0dda4ab828223834244efb23a422207626b7b0")},
    {44000, uint256S("f3ca8f7322b1fb694699ba0e1500d0efdfb5473d06e2ea237ed57c80bbfc99c8")},
	{64000, uint256S("8ad445ba3454f6b9645a84e28c8b6667a5b5542a4910da34a0d5ee283185e26f")},
	{104480, uint256S("ef5cae12f53ac57d365c255a6665461dbf78725049b42acfde7d1f75f97ea826")},
	{132000, uint256S("6580145f6a1c7b8bf7e59f018f238dfa578d0f54958db8bd3e8cf60826b10332")},
	{220000, uint256S("c0b67e4f7931e7af498c613cb4448ceee331e2ef3b78c0e9f83b79812f438ae9")},
	{300000, uint256S("974c5af0ddeba4168d4c1727432607597c788070c13f0bb3ee6c655209405d95")},
	{396000, uint256S("ccde21928075cfd77f7c88f8373f028db9e52c07844f9c227f5a1ec61063f4c2")},
	{484000, uint256S("3ed37d8ab174531c5a7d4af54d389f7e804184dbbb9dbde0f494516474430743")},
	{500512, uint256S("978590765061b3a755e14a598cc6f24b380cbcbd1564b31c126ddc7601b41320")}
};

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
	1649047095, // * UNIX timestamp of last checkpoint block
	1472523,    // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the UpdateTip debug.log lines)
	4500        // * estimated number of transactions per day after checkpoint
};


static Checkpoints::MapCheckpoints mapCheckpointsTestnet = {
	{0, uint256S("00000ee2d6a0046735414b92173442086f02af56a2a62208412c0d091a6461f5")},
	{1, uint256S("00000e594167d5c1facc7c67bd72c76ef9a57d0a2803d6b26cbb06791d745188")},
	{2, uint256S("00000bfbc30d62fc31c6faab7b2fdac2ed8e0f9272e168015ee199f5e7c6f266")}
};

static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
	1629209498,
	3,
	1440};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest = {{0, uint256S("0x001")}};

static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    1618207622,
    1,
    100};


class CMainParams : public CChainParams
{
public:

    CMainParams()
    {
        strNetworkID = "main";

        consensus.strGenesisPubKeyHex = "04c10e83b2703ccf322f7dbd62dd5855ac7c10bd055814ce121ba32607d573b8810c02c0582aed05b4deb9c4b77b26d92428c61256cd42774babea0a073b2ed0c9";

        genesis = CreateGenesisBlock(1618207620, 4293940, 0x1e0ffff0, 1, 250 * COIN, consensus.strGenesisPubKeyHex);
        consensus.hashGenesisBlock = genesis.GetHash();

        generateGenesis(genesis, strNetworkID);

        assert(consensus.hashGenesisBlock == uint256S("00000e2864f744c05d9fc94c4f02f0810927ab2100abaae3f75e87f915dbcec1"));
        assert(genesis.hashMerkleRoot == uint256S("a6b5cf9995c2ec8d91e1ff76ec33bca9287f4088586747cf4a475959e41f4750"));

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.powLimit   = ~UINT256_ZERO >> 20;   // QRAX starting difficulty is 1 / 2^12
        consensus.posLimitV1 = ~UINT256_ZERO >> 24;
        consensus.posLimitV2 = ~UINT256_ZERO >> 20;
        consensus.nBudgetCycleBlocks = 44000;       // approx. 1 every 30 days
        consensus.nBudgetFeeConfirmations = 6;      // Number of confirmations for the finalization fee
        consensus.nCoinbaseMaturity = 20; // 100
        consensus.nFutureTimeDriftPoW = 7200;
        consensus.nFutureTimeDriftPoS = 180;
        consensus.nMaxMoneyOut = 10000000 * COIN;
        consensus.nPoolMaxTransactions = 3;
        consensus.nProposalEstablishmentTime = 60 * 60 * 24;    // must be at least a day old to make it into a budget
        consensus.nStakeMinAge = 60 * 60;
        consensus.nStakeMinDepth = 100;
        consensus.nTargetTimespan = 40 * 60;
        consensus.nTargetTimespanV2 = 30 * 60;
        consensus.nTargetSpacing = 1 * 60;
        consensus.nTimeSlotLength = 15;
        consensus.nMaxProposalPayments = 6;
        consensus.nRewardHalvingBlocks = 264000;


        // spork keys
        consensus.strSporkPubKey = "04c9c8516a1bd7f3d36fcccdd275e6c152f1119d549eaeaa0492297774afb3db48761a1f8764f9445e11b07521557bfa4a55fb9843d6bc55aa0da16440ebb10cde";

        // multiming program keys
        consensus.strMultiMiningProgramKey = "04a61b31f0b5a0f95a3563f855cf825b5d68dfd008fcc26b0c3f1615cbba8d1b15bff7ce82df4dbbfc6d1201b8b7a224de92794847949550df30b6a39bbef30caf";


        // Network upgrades

		consensus.vUpgrades[Consensus::BASE_NETWORK].nActivationHeight				= Consensus::NetworkUpgrade::ALWAYS_ACTIVE;
		consensus.vUpgrades[Consensus::ACTIVATE_POS].nActivationHeight				= 44000; //
		consensus.vUpgrades[Consensus::ACTIVATE_NEW_COLDSTAKE].nActivationHeight    = 512000;
		consensus.vUpgrades[Consensus::ACTIVATE_NEW_FEE_RULES].nActivationHeight	= 512000;
		consensus.vUpgrades[Consensus::ACTIVATE_ASSET_VALIDATION].nActivationHeight = Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
		consensus.vUpgrades[Consensus::ACTIVATE_HUSH].nActivationHeight				= Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
		consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nActivationHeight			= Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */

        pchMessageStart[0] = 0xb5;
        pchMessageStart[1] = 0xa1;
        pchMessageStart[2] = 0xf0;
        pchMessageStart[3] = 0xfe;
        nDefaultPort = 18962;

        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.emplace_back("dnsseed.qrax.net", "dnsseed.qrax.net", true);    // DNS Seeder
        vSeeds.emplace_back("dnsseed1.qrax.net", "dnsseed1.qrax.net", true);    // DNS Seeder
        vSeeds.emplace_back("dnsseed2.qrax.net", "dnsseed2.qrax.net", true);    // DNS Seeder
        vSeeds.emplace_back("dnsseed3.qrax.net", "dnsseed3.qrax.net", true);    // DNS Seeder
        vSeeds.emplace_back("dnsseed4.qrax.net", "dnsseed4.qrax.net", true);    // DNS Seeder
        vSeeds.emplace_back("dnsseed5.qrax.net", "dnsseed5.qrax.net", true);    // DNS Seeder

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 58); // starting with 'Q'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 13); // starting with '6'
        base58Prefixes[STAKING_ADDRESS] = std::vector<unsigned char>(1, 63);     // starting with 'S'

        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 212);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x02, 0x2D, 0x25, 0x33};
        base58Prefixes[EXT_SECRET_KEY] = {0x02, 0x21, 0x31, 0x2B};

        // BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md
        base58Prefixes[EXT_COIN_TYPE] = {0x80, 0x00, 0x00, 0x77};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        // Sapling
        bech32HRPs[SAPLING_PAYMENT_ADDRESS]      = "qrax";
        bech32HRPs[SAPLING_FULL_VIEWING_KEY]     = "qraxviews";
        bech32HRPs[SAPLING_INCOMING_VIEWING_KEY] = "qraxs";
        bech32HRPs[SAPLING_EXTENDED_SPEND_KEY]   = "qrax-secret-spending-key-main";
        bech32HRPs[SAPLING_EXTENDED_FVK]         = "qraxviews";
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }

};

/**
 * Testnet (v5)
 */
class CTestNetParams : public CChainParams
{
public:
    CTestNetParams()
    {
        strNetworkID = "test";

        consensus.strGenesisPubKeyHex = "04c10e83b2703ccf322f7dbd62dd5855ac7c10bd055814ce121ba32607d573b8810c02c0582aed05b4deb9c4b77b26d92428c61256cd42774babea0a073b2ed0c9";

        genesis = CreateGenesisBlock(1618207621, 980097, 0x1e0ffff0, 1, 250 * COIN, consensus.strGenesisPubKeyHex);
        consensus.hashGenesisBlock = genesis.GetHash();

        generateGenesis(genesis, strNetworkID);

        assert(consensus.hashGenesisBlock == uint256S("00000ee2d6a0046735414b92173442086f02af56a2a62208412c0d091a6461f5"));
        assert(genesis.hashMerkleRoot == uint256S("a6b5cf9995c2ec8d91e1ff76ec33bca9287f4088586747cf4a475959e41f4750"));

        consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.powLimit   = ~UINT256_ZERO >> 20;   // QRAX starting difficulty is 1 / 2^12
        consensus.posLimitV1 = ~UINT256_ZERO >> 24;
        consensus.posLimitV2 = ~UINT256_ZERO >> 20;
        consensus.nBudgetCycleBlocks = 1440;         // approx 1 cycle per day
        consensus.nBudgetFeeConfirmations = 3;      // (only 8-blocks window for finalization on testnet)
        consensus.nCoinbaseMaturity = 15;
        consensus.nFutureTimeDriftPoW = 7200;
        consensus.nFutureTimeDriftPoS = 180;
        consensus.nMaxMoneyOut = 20000000 * COIN;
        consensus.nPoolMaxTransactions = 2;
        consensus.nMasternodeCountDrift = 20;       // num of MN we allow the see-saw payments to be off by

        consensus.nProposalEstablishmentTime = 60 * 5;  // at least 5 min old to make it into a budget
        consensus.nStakeMinAge = 60 * 60;
        consensus.nStakeMinDepth = 100;
        consensus.nTargetTimespan = 40 * 60;
        consensus.nTargetTimespanV2 = 30 * 60;
        consensus.nTargetSpacing = 1 * 60;
        consensus.nTimeSlotLength = 15;
        consensus.nMaxProposalPayments = 20;
        consensus.nRewardHalvingBlocks = 2880;

        // spork keys
        consensus.strSporkPubKey = "04f428a61d93b2d53ac61a43fc339efd9fddb7fc819f77e928c4fc53c326c17d0bda9c409d0292f2bbce3ea4bf7da610fba647da71a042c16d80d2b10ed4d4bc50";

        // multiming program keys
        consensus.strMultiMiningProgramKey = "043737fa9e160a62ee2ae0b5d794305c3be8d7cc77aac4763f563dac08aded60bb1e65067e4208e6e799b52d4e329687483afdbbb1bccaeef094d7fa29b1eae2dd";

        // Network upgrades
		consensus.vUpgrades[Consensus::BASE_NETWORK].nActivationHeight				= Consensus::NetworkUpgrade::ALWAYS_ACTIVE;
		consensus.vUpgrades[Consensus::ACTIVATE_POS].nActivationHeight				= 200;
		consensus.vUpgrades[Consensus::ACTIVATE_NEW_COLDSTAKE].nActivationHeight    = 2000;
		consensus.vUpgrades[Consensus::ACTIVATE_NEW_FEE_RULES].nActivationHeight	= 306500;
		consensus.vUpgrades[Consensus::ACTIVATE_ASSET_VALIDATION].nActivationHeight = Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
		consensus.vUpgrades[Consensus::ACTIVATE_HUSH].nActivationHeight				= Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
		consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nActivationHeight			= Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */

        pchMessageStart[0] = 0xe4;
        pchMessageStart[1] = 0xd5;
        pchMessageStart[2] = 0xc4;
        pchMessageStart[3] = 0xb9;
        nDefaultPort = 18964;

        vFixedSeeds.clear();
        vSeeds.clear();


        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("95.217.17.253", "95.217.17.253", true);
        vSeeds.emplace_back("95.217.20.59", "95.217.20.59", true);
        vSeeds.emplace_back("159.253.19.179", "159.253.19.179", true);
        vSeeds.emplace_back("92.242.62.104", "92.242.62.104", true);

        vSeeds.emplace_back("95.216.212.34", "95.216.212.34", true);
        vSeeds.emplace_back("135.181.144.246", "135.181.144.246", true);
        vSeeds.emplace_back("95.216.213.185", "95.216.213.185", true);
        vSeeds.emplace_back("135.181.144.251", "135.181.144.251", true);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 139); // Testnet qrax addresses start with 'x' or 'y'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);  // Testnet qrax script addresses start with '8' or '9'
        base58Prefixes[STAKING_ADDRESS] = std::vector<unsigned char>(1, 73);     // starting with 'W'
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);     // Testnet private keys start with '9' or 'c' (Bitcoin defaults)

		// Testnet qrax BIP32 pubkeys start with 'DRKV'
        base58Prefixes[EXT_PUBLIC_KEY] = {0x3a, 0x80, 0x61, 0xa0};
		// Testnet qrax BIP32 prvkeys start with 'DRKP'
        base58Prefixes[EXT_SECRET_KEY] = {0x3a, 0x80, 0x58, 0x37};
		// Testnet qrax BIP44 coin type is '1' (All coin's testnet default)
        base58Prefixes[EXT_COIN_TYPE] = {0x80, 0x00, 0x00, 0x01};


        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        // Sapling
        bech32HRPs[SAPLING_PAYMENT_ADDRESS]      = "qraxtestsapling";
        bech32HRPs[SAPLING_FULL_VIEWING_KEY]     = "qraxfullviewtestsapling";
        bech32HRPs[SAPLING_INCOMING_VIEWING_KEY] = "qraxktestsapling";
        bech32HRPs[SAPLING_EXTENDED_SPEND_KEY]   = "qrax-secret-spending-key-test";
        bech32HRPs[SAPLING_EXTENDED_FVK]         = "qraxextviewtestsapling";
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams
{
public:
    CRegTestParams()
    {
        strNetworkID = "regtest";
        consensus.strGenesisPubKeyHex = "04c10e83b2703ccf322f7dbd62dd5855ac7c10bd055814ce121ba32607d573b8810c02c0582aed05b4deb9c4b77b26d92428c61256cd42774babea0a073b2ed0c9";

        genesis = CreateGenesisBlock(1618207622, 4327782, 0x1e0ffff0, 1, 250 * COIN, consensus.strGenesisPubKeyHex);

        consensus.hashGenesisBlock = genesis.GetHash();

        generateGenesis(genesis, strNetworkID);

        assert(consensus.hashGenesisBlock == uint256S("000005f32049fdad20ebfaa813a666116fc2f0a0ab257685126fbc83ec6688a1"));
        assert(genesis.hashMerkleRoot == uint256S("a6b5cf9995c2ec8d91e1ff76ec33bca9287f4088586747cf4a475959e41f4750"));


        consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.powLimit   = ~UINT256_ZERO >> 20;   // QRAX starting difficulty is 1 / 2^12
        consensus.posLimitV1 = ~UINT256_ZERO >> 24;
        consensus.posLimitV2 = ~UINT256_ZERO >> 20;
        consensus.nBudgetCycleBlocks = 144;         // approx 10 cycles per day
        consensus.nBudgetFeeConfirmations = 3;      // (only 8-blocks window for finalization on regtest)
        consensus.nCoinbaseMaturity = 100;
        consensus.nFutureTimeDriftPoW = 7200;
        consensus.nFutureTimeDriftPoS = 180;
        consensus.nMaxMoneyOut = 43199500 * COIN;
        consensus.nPoolMaxTransactions = 2;
        consensus.nProposalEstablishmentTime = 60 * 5;  // at least 5 min old to make it into a budget
        consensus.nStakeMinAge = 0;
        consensus.nStakeMinDepth = 2;
        consensus.nTargetTimespan = 40 * 60;
        consensus.nTargetTimespanV2 = 30 * 60;
        consensus.nTargetSpacing = 1 * 60;
        consensus.nTimeSlotLength = 15;
        consensus.nRewardHalvingBlocks = 144;
        consensus.nMaxProposalPayments = 20;

        /* Spork Key for RegTest:
        WIF private key: 932HEevBSujW2ud7RfB1YF91AFygbBRQj3de3LyaCRqNzKKgWXi
        private key hex: bd4960dcbd9e7f2223f24e7164ecb6f1fe96fc3a416f5d3a830ba5720c84b8ca
        Address: yCvUVd72w7xpimf981m114FSFbmAmne7j9
        */
        consensus.strSporkPubKey = "043969b1b0e6f327de37f297a015d37e2235eaaeeb3933deecd8162c075cee0207b13537618bde640879606001a8136091c62ec272dd0133424a178704e6e75bb7";

        consensus.strMultiMiningProgramKey = "";

        // Network upgrades
		consensus.vUpgrades[Consensus::BASE_NETWORK].nActivationHeight					= Consensus::NetworkUpgrade::ALWAYS_ACTIVE;
		consensus.vUpgrades[Consensus::ACTIVATE_POS].nActivationHeight					= 360;
		consensus.vUpgrades[Consensus::ACTIVATE_NEW_COLDSTAKE].nActivationHeight        = 360*2;
		consensus.vUpgrades[Consensus::ACTIVATE_NEW_FEE_RULES].nActivationHeight		= 360*2;
		consensus.vUpgrades[Consensus::ACTIVATE_ASSET_VALIDATION].nActivationHeight     = Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
		consensus.vUpgrades[Consensus::ACTIVATE_HUSH].nActivationHeight					= Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;
		consensus.vUpgrades[Consensus::UPGRADE_TESTDUMMY].nActivationHeight				= Consensus::NetworkUpgrade::NO_ACTIVATION_HEIGHT;


        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */

        pchMessageStart[0] = 0xa2;
        pchMessageStart[1] = 0xce;
        pchMessageStart[2] = 0x7d;
        pchMessageStart[3] = 0xab;
        nDefaultPort = 18966;


		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 139); // Testnet qrax addresses start with 'x' or 'y'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);  // Testnet qrax script addresses start with '8' or '9'
        base58Prefixes[STAKING_ADDRESS] = std::vector<unsigned char>(1, 73);     // starting with 'W'
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);     // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
		// Testnet qrax BIP32 pubkeys start with 'DRKV'
        base58Prefixes[EXT_PUBLIC_KEY] = {0x3a, 0x80, 0x61, 0xa0};
		// Testnet qrax BIP32 prvkeys start with 'DRKP'
        base58Prefixes[EXT_SECRET_KEY] = {0x3a, 0x80, 0x58, 0x37};
		// Testnet qrax BIP44 coin type is '1' (All coin's testnet default)
        base58Prefixes[EXT_COIN_TYPE] = {0x80, 0x00, 0x00, 0x01};

        // Sapling
		bech32HRPs[SAPLING_PAYMENT_ADDRESS]      = "qtestsapling";
		bech32HRPs[SAPLING_FULL_VIEWING_KEY]     = "qviewtestsapling";
		bech32HRPs[SAPLING_INCOMING_VIEWING_KEY] = "qrxktestsapling";
		bech32HRPs[SAPLING_EXTENDED_SPEND_KEY]   = "qp-secret-spending-key-test";
		bech32HRPs[SAPLING_EXTENDED_FVK]         = "qxviewtestsapling";
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params()
{
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateNetworkUpgradeParameters(Consensus::UpgradeIndex idx, int nActivationHeight)
{
    globalChainParams->UpdateNetworkUpgradeParameters(idx, nActivationHeight);
}
