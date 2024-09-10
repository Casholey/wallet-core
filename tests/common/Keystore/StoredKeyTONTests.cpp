// SPDX-License-Identifier: Apache-2.0
//
// Copyright © 2017 Trust Wallet.

#include "Keystore/StoredKey.h"

#include "Coin.h"
#include "Data.h"
#include "HexCoding.h"
#include "StoredKeyConstants.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace TW::Keystore::tests {

using namespace std;

const auto gTONMnemonic = "protect drill sugar gallery note admit input wrist chicken swarm scheme hedgehog orbit ritual glove ski buddy slogan fragile sun delay toy lucky require";
// The following TON mnemonic requires a passphrase to be used that we don't support right now.
const auto gTONInvalidMnemonic = "mimic close sibling chair shuffle goat fashion chunk increase tennis scene ceiling divert cross treat happy soccer sample umbrella oyster advance quality perfect call";
const auto gTONPrivateKey = "cdcea50b87d3f1ca859e7b2bdf9a5339b7b6804b5c70ac85198829f9607dc43b";
const auto gTONPublicKey = "9016f03f9cfa4e183707761f25407e0e1975194a33a56b3e8d2c26f2438fa3d1";
const auto gBounceableAddress = "EQBlm676c6vy6Q9Js732pvf3ivfmIkVc0MVDQy-F6NAFJYPh";
const auto gNonBounceableAddress = "UQBlm676c6vy6Q9Js732pvf3ivfmIkVc0MVDQy-F6NAFJd4k";

TEST(StoredKeyTON, CreateWithTonMnemonicAddDefault) {
    auto key = StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeTON, gTONMnemonic);
    EXPECT_EQ(key.type, StoredKeyType::tonMnemonicPhrase);
    const Data& mnemo2Data = key.payload.decrypt(gPassword);
    EXPECT_EQ(string(mnemo2Data.begin(), mnemo2Data.end()), string(gTONMnemonic));
    EXPECT_EQ(key.accounts.size(), 1ul);
    EXPECT_EQ(key.accounts[0].coin, TWCoinTypeTON);
    EXPECT_EQ(key.accounts[0].address, "UQBlm676c6vy6Q9Js732pvf3ivfmIkVc0MVDQy-F6NAFJd4k");
    EXPECT_EQ(key.accounts[0].publicKey, gTONPublicKey);
    EXPECT_EQ(key.accounts[0].extendedPublicKey, "");
    EXPECT_EQ(key.accounts[0].derivationPath.string(), "");
    EXPECT_EQ(key.accounts[0].derivation, TWDerivationDefault);
    EXPECT_EQ(hex(key.privateKey(TWCoinTypeTON, gPassword).bytes), gTONPrivateKey);
    EXPECT_EQ(key.payload.params.cipher(), "aes-128-ctr");

    const auto json = key.json();
    EXPECT_EQ(json["name"], gName);
    EXPECT_EQ(json["type"], "ton-mnemonic");
    EXPECT_EQ(json["version"], 3);
}

TEST(StoredKeyTON, CreateWithTonMnemonicInvalid) {
    EXPECT_THROW(
        StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeTON, gTONInvalidMnemonic),
        std::invalid_argument
    );
}

TEST(StoredKeyTON, CreateWithTonMnemonicInvalidCoinType) {
    EXPECT_THROW(
        StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeBitcoin, gTONMnemonic),
        std::invalid_argument
    );
}

TEST(StoredKeyTON, CreateWithMnemonicAddDefaultAddressAes256) {
    auto key = StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeTON, gTONMnemonic, TWStoredKeyEncryptionAes256Ctr);
    EXPECT_EQ(key.type, StoredKeyType::tonMnemonicPhrase);
    const Data& mnemo2Data = key.payload.decrypt(gPassword);
    EXPECT_EQ(string(mnemo2Data.begin(), mnemo2Data.end()), string(gTONMnemonic));
    EXPECT_EQ(key.accounts.size(), 1ul);
    EXPECT_EQ(key.accounts[0].coin, TWCoinTypeTON);
    EXPECT_EQ(key.accounts[0].address, "UQBlm676c6vy6Q9Js732pvf3ivfmIkVc0MVDQy-F6NAFJd4k");
    EXPECT_EQ(key.payload.params.cipher(), "aes-256-ctr");
}

TEST(StoredKeyTON, HDWalletNotSupported) {
    auto key = StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeTON, gTONMnemonic);
    EXPECT_THROW(key.wallet(gPassword), std::invalid_argument);
}

TEST(StoredKeyTON, AddRemoveAccount) {
    auto key = StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeTON, gTONMnemonic);
    EXPECT_EQ(key.accounts.size(), 1ul);

    // Add another dummy (doesn't belong to the mnemonic) TON account.
    {
        const DerivationPath derivationPath {};
        const auto publicKey = "b191d35f81aa8b144aa91c90a6b887e0b165ad9c2933b1c5266eb5c4e8bea241";
        const auto extendedPublicKey = "";
        key.addAccount("UQDSRYDMMez8BdcOuPEiaR6aJZpO6EjlIwmOBFn14mMbnRah", TWCoinTypeTON, TWDerivationDefault, derivationPath, publicKey, extendedPublicKey);
        EXPECT_EQ(key.accounts.size(), 2ul);
    }

    key.removeAccount(TWCoinTypeTON, TWDerivationDefault);
    EXPECT_EQ(key.accounts.size(), 0ul);
}

TEST(StoredKeyTON, FixAddressHasNoEffect) {
    // `StoredKey::createWithTonMnemonicAddDefaultAddress` derives the correct address.
    auto key = StoredKey::createWithTonMnemonicAddDefaultAddress(gName, gPassword, TWCoinTypeTON, gTONMnemonic);
    EXPECT_EQ(key.accounts.size(), 1ul);

    key.fixAddresses(gPassword);
    EXPECT_EQ(key.accounts[0].address, gNonBounceableAddress);
}

TEST(StoredKeyTON, FixAddress) {
    auto key = StoredKey::createWithTonMnemonic(gName, gPassword, gTONMnemonic);
    EXPECT_EQ(key.accounts.size(), 0ul);

    // Add an account with an invalid address manually.
    {
        const DerivationPath derivationPath {};
        const auto publicKey = gTONPublicKey;
        const auto extendedPublicKey = "";
        const auto invalidAddress = "INVALID_ADDRESS";
        key.addAccount(invalidAddress, TWCoinTypeTON, TWDerivationDefault, derivationPath, publicKey, extendedPublicKey);
        EXPECT_EQ(key.accounts.size(), 1ul);
    }

    key.fixAddresses(gPassword);
    EXPECT_EQ(key.accounts.size(), 1ul);
    EXPECT_EQ(key.accounts[0].coin, TWCoinTypeTON);
    // Address should be fixed to a valid non-bounceable address.
    EXPECT_EQ(key.accounts[0].address, gNonBounceableAddress);
}

TEST(StoredKeyTON, UpdateAddress) {
    auto key = StoredKey::createWithTonMnemonic(gName, gPassword, gTONMnemonic);
    EXPECT_EQ(key.accounts.size(), 0ul);

    // Add an account with a bounceable (EQ) address.
    {
        const DerivationPath derivationPath {};
        const auto publicKey = gTONPublicKey;
        const auto extendedPublicKey = "";
        const auto invalidAddress = gBounceableAddress;
        key.addAccount(invalidAddress, TWCoinTypeTON, TWDerivationDefault, derivationPath, publicKey, extendedPublicKey);
        EXPECT_EQ(key.accounts.size(), 1ul);
    }

    key.updateAddress(TWCoinTypeTON);
    EXPECT_EQ(key.accounts.size(), 1ul);
    EXPECT_EQ(key.accounts[0].coin, TWCoinTypeTON);
    // Address should be fixed to a valid non-bounceable address.
    EXPECT_EQ(key.accounts[0].address, gNonBounceableAddress);
}

TEST(StoredKeyTON, LoadNonexistent) {
    ASSERT_THROW(StoredKey::load(testDataPath("nonexistent.json")), invalid_argument);
}

TEST(StoredKeyTON, LoadTonMnemonic) {
    const auto key = StoredKey::load(testDataPath("ton-wallet.json"));
    EXPECT_EQ(key.type, StoredKeyType::tonMnemonicPhrase);
    EXPECT_EQ(key.id, "f7a2172e-fb7a-427a-8526-99779fc47c0a");
    EXPECT_EQ(key.name, "Test TON Account");

    const auto data = key.payload.decrypt(gPassword);
    const auto mnemonic = string(reinterpret_cast<const char*>(data.data()), data.size());
    EXPECT_EQ(mnemonic, gTONMnemonic);

    EXPECT_EQ(key.accounts[0].coin, TWCoinTypeTON);
    EXPECT_EQ(key.accounts[0].derivationPath.string(), "");
    EXPECT_EQ(key.accounts[0].address, gNonBounceableAddress);
    EXPECT_EQ(key.accounts[0].publicKey, gTONPublicKey);
}

TEST(StoredKeyTON, InvalidPassword) {
    const auto key = StoredKey::load(testDataPath("ton-wallet.json"));

    ASSERT_THROW(key.payload.decrypt(TW::data("INVALID PASSWORD")), DecryptionError);
}

} // namespace TW::Keystore