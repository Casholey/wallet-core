// Copyright © 2017-2023 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#include "TransactionSigner.h"

#include "SignatureBuilder.h"

#include "../BitcoinDiamond/Transaction.h"
#include "../BitcoinDiamond/TransactionBuilder.h"
#include "../Groestlcoin/Transaction.h"
#include "../Verge/Transaction.h"
#include "../Verge/TransactionBuilder.h"
#include "../Zcash/Transaction.h"
#include "../Zcash/TransactionBuilder.h"
#include "../Zen/TransactionBuilder.h"

namespace TW::Bitcoin {

template <typename Transaction, typename TransactionBuilder>
TransactionPlan TransactionSigner<Transaction, TransactionBuilder>::plan(const SigningInput& input) {
    return TransactionBuilder::plan(input);
}

template <typename Transaction, typename TransactionBuilder>
Result<Transaction, Common::Proto::SigningError> TransactionSigner<Transaction, TransactionBuilder>::sign(const SigningInput& input, bool estimationMode, std::optional<SignaturePubkeyList> optionalExternalSigs) {
    TransactionPlan plan;
    if (input.plan.has_value()) {
        plan = input.plan.value();
    } else {
        plan = TransactionBuilder::plan(input);
    }

    auto callRust = input.isItBrcOperation;
    if (true) {
        Proto::SigningInput proto;
        for (auto key: input.privateKeys) {
            proto.add_private_key(key.bytes.data(), key.bytes.size());
        }

        for (auto utxo: input.utxos) {
            auto& protoUtxo = *proto.add_utxo();
            protoUtxo.set_amount(utxo.amount);
            protoUtxo.set_script(utxo.script.bytes.data(), utxo.script.bytes.size());
            protoUtxo.set_variant(utxo.variant);

            Proto::OutPoint out;
            out.set_index(utxo.outPoint.index);
            out.set_hash(std::string(utxo.outPoint.hash.begin(), utxo.outPoint.hash.end()));
            *protoUtxo.mutable_out_point() = out;
        }

        *proto.mutable_plan() = plan.proto();

        Proto::SigningOutput output;
        auto serializedInput = data(proto.SerializeAsString());
        Rust::CByteArrayWrapper res = Rust::tw_taproot_build_and_sign_transaction(serializedInput.data(), serializedInput.size());
        output.ParseFromArray(res.data.data(), static_cast<int>(res.data.size()));
        auto protoTx = output.transaction();

        auto tx = Transaction();
        tx._version = protoTx.version();
        tx.lockTime = protoTx.locktime();

        for (auto protoInput: protoTx.inputs()) {
            TW::Data vec = parse_hex(protoInput.previousoutput().hash());
            std::array<byte, 32> hash;
            std::copy(vec.begin(), vec.end(), hash.begin());

            auto out = TW::Bitcoin::OutPoint();
            out.hash = hash;
            out.index = protoInput.previousoutput().index();
            out.sequence = protoInput.previousoutput().sequence();

            auto script = Script(parse_hex(protoInput.script()));
            auto input = TW::Bitcoin::TransactionInput(out, script, protoInput.sequence());
            tx.inputs.push_back(input);
        }

        for (auto protoOutput: protoTx.outputs()) {
            auto script = Script(parse_hex(protoOutput.script()));
            auto output = TransactionOutput(protoOutput.value(), script);
            tx.outputs.push_back(output);
        }

        return Result<Transaction, Common::Proto::SigningError>(tx);
    }

    auto tx_result = TransactionBuilder::template build<Transaction>(plan, input);
    if (!tx_result) {
        return Result<Transaction, Common::Proto::SigningError>::failure(tx_result.error());
    }
    Transaction transaction = tx_result.payload();
    SigningMode signingMode =
        estimationMode ? SigningMode_SizeEstimationOnly : optionalExternalSigs.has_value() ? SigningMode_External
                                                                                           : SigningMode_Normal;
    SignatureBuilder<Transaction> signer(std::move(input), plan, transaction, signingMode, optionalExternalSigs);
    return signer.sign();
}

template <typename Transaction, typename TransactionBuilder>
Result<HashPubkeyList, Common::Proto::SigningError> TransactionSigner<Transaction, TransactionBuilder>::preImageHashes(const SigningInput& input) {
    TransactionPlan plan;
    if (input.plan.has_value()) {
        plan = input.plan.value();
    } else {
        plan = TransactionBuilder::plan(input);
    }
    auto tx_result = TransactionBuilder::template build<Transaction>(plan, input);
    if (!tx_result) {
        return Result<HashPubkeyList, Common::Proto::SigningError>::failure(tx_result.error());
    }
    Transaction transaction = tx_result.payload();
    SignatureBuilder<Transaction> signer(std::move(input), plan, transaction, SigningMode_HashOnly);
    auto signResult = signer.sign();
    if (!signResult) {
        return Result<HashPubkeyList, Common::Proto::SigningError>::failure(signResult.error());
    }
    return Result<HashPubkeyList, Common::Proto::SigningError>::success(signer.getHashesForSigning());
}

// Explicitly instantiate a Signers for compatible transactions.
template class Bitcoin::TransactionSigner<Bitcoin::Transaction, TransactionBuilder>;
template class Bitcoin::TransactionSigner<Zcash::Transaction, Zcash::TransactionBuilder>;
template class Bitcoin::TransactionSigner<Bitcoin::Transaction, Zen::TransactionBuilder>;
template class Bitcoin::TransactionSigner<Groestlcoin::Transaction, TransactionBuilder>;
template class Bitcoin::TransactionSigner<Verge::Transaction, Verge::TransactionBuilder>;
template class Bitcoin::TransactionSigner<BitcoinDiamond::Transaction, BitcoinDiamond::TransactionBuilder>;

} // namespace TW::Bitcoin
