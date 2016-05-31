/*
    This file is part of etherwall.
    etherwall is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    etherwall is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with etherwall. If not, see <http://www.gnu.org/licenses/>.
*/
/** @file types.cpp
 * @author Ales Katona <almindor@gmail.com>
 * @date 2015
 *
 * Types implementation
 */

#include "types.h"
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QJsonDocument>
#include <QApplication>
#include <QDebug>

namespace Etherwall {

    const QString DefaultIPCPath(bool testnet) {
    #ifdef Q_OS_WIN32
        return "\\\\.\\pipe\\geth.ipc";
    #else
    #ifdef Q_OS_MACX
        const QString base_path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Library/Ethereum/";
    #else
        const QString base_path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.ethereum/";
    #endif
        const QString mid_fix = testnet ? "/testnet" : "";
        return base_path + mid_fix + "/geth.ipc";
    #endif
    }

    const QString DefaultGethPath() {
#ifdef Q_OS_WIN32
        return QApplication::applicationDirPath() + "/geth";
#else
#ifdef Q_OS_MACX
        return QApplication::applicationDirPath() + "/geth";
#else
        return "/usr/bin/geth";
#endif
#endif
    }

// ***************************** LogInfo ***************************** //

    LogInfo::LogInfo(const QString& info, LogSeverity sev) : fMsg(info), fDate(QDateTime::currentDateTime()), fSeverity(sev)
    {

    }

    const QVariant LogInfo::value(int role) const {
        switch ( role ) {
            case MsgRole: return QVariant(fMsg);
            case DateRole: return QVariant(fDate);
            case SeverityRole: return QVariant(getSeverityString());
        }

        return QVariant();
    }

    const QString LogInfo::getSeverityString() const {
        switch ( fSeverity ) {
            case LS_Debug: return QString("Debug");
            case LS_Info: return QString("Info");
            case LS_Warning: return QString("Warning");
            case LS_Error: return QString("Error");
        }

        return QString("Unknown");
    }

    // ***************************** Denomination ***************************** //

    CurrencyInfo::CurrencyInfo( const QString name, const float price ) : fName(name), fPrice(price) {
    }

    const QVariant CurrencyInfo::value(const int role) const {
        switch ( role ) {
        case NameRole: return QVariant(fName);
        case PriceRole: return QVariant(fPrice);
        }

        return QVariant();
    }

    double CurrencyInfo::recalculate(const float ether) const {
        return ether * fPrice;
    }


// ***************************** TransactionInfo ***************************** //

    static int ACC_INDEX = 0;

    AccountInfo::AccountInfo(const QString& hash, const QString& balance, quint64 transCount) :
        fIndex(ACC_INDEX++), fHash(hash), fBalance(balance), fTransCount(transCount)
    {
        const QSettings settings;

        if ( settings.contains("alias/" + hash) ) {
            fAlias = settings.value("alias/" + hash, QString()).toString();
        }
    }

    const QVariant AccountInfo::value(const int role) const {
        switch ( role ) {
        case HashRole: return QVariant(fHash);
        case BalanceRole: return QVariant(fBalance);
        case TransCountRole: return QVariant(fTransCount);
        case SummaryRole: return QVariant(value(AliasRole).toString() + " [" + fBalance + "]");
        case AliasRole: return QVariant(fAlias.isEmpty() ? fHash : fAlias);
        case IndexRole: return QVariant(fIndex);
        }

        return QVariant();
    }

    void AccountInfo::setBalance(const QString& balance) {
        fBalance = balance;
    }

    void AccountInfo::setTransactionCount(quint64 count) {
        fTransCount = count;
    }

    void AccountInfo::alias(const QString& name) {
        QSettings settings;

        settings.setValue("alias/" + fHash, name);
        fAlias = name;
    }

// ***************************** TransactionInfo ***************************** //

    TransactionInfo::TransactionInfo() : fSenderAlias(), fReceiverAlias()
    {
        fValue = "0x0";
        fBlockNumber = 0;
        fTransactionIndex = 0;
    }

    TransactionInfo::TransactionInfo(const QJsonObject& source) : fSenderAlias(), fReceiverAlias()
    {
        init(source);
    }

    TransactionInfo::TransactionInfo(const QString& hash, quint64 blockNum) : fHash(hash), fBlockNumber(blockNum), fSenderAlias(), fReceiverAlias()
    {
    }

    const QVariant TransactionInfo::value(const int role) const {
        switch ( role ) {
            case THashRole: return QVariant(fHash);
            case NonceRole: return QVariant(fNonce);
            case SenderRole: return QVariant(fSender);
            case ReceiverRole: return QVariant(fReceiver);
            case ValueRole: return QVariant(fValue);
            case BlockNumberRole: return QVariant(fBlockNumber);
            case BlockHashRole: return QVariant(fBlockHash);
            case TransactionIndexRole: return QVariant(fTransactionIndex);
            case GasRole: return QVariant(fGas);
            case GasPriceRole: return QVariant(fGasPrice);
            case InputRole: return QVariant(fInput);
            case SenderAliasRole: return QVariant(fSenderAlias.isEmpty() ? fSender : fSenderAlias);
            case ReceiverAliasRole: return QVariant(fReceiverAlias.isEmpty() ? fReceiver : fReceiverAlias);
        }

        return QVariant();
    }

    quint64 TransactionInfo::getBlockNumber() const {
        return fBlockNumber;
    }

    void TransactionInfo::setBlockNumber(quint64 num) {
        fBlockNumber = num;
    }

    const QString TransactionInfo::getHash() const {
        return fHash;
    }

    void TransactionInfo::setHash(const QString& hash) {
        fHash = hash;
    }

    void TransactionInfo::init(const QString& from, const QString& to, const QString& value, const QString& gas) {
        fSender = from;
        fReceiver = to;
        fNonce = 0;
        fValue = Helpers::formatEtherStr(value);
        if ( !gas.isEmpty() ) {
            fGas = gas;
        }

        lookupAccountAliases();
    }

    void TransactionInfo::init(const QJsonObject source) {
        fHash = source.value("hash").toString("invalid");
        fNonce = Helpers::toQUInt64(source.value("nonce"));
        fSender = source.value("from").toString("invalid");
        fReceiver = source.value("to").toString("invalid");
        fBlockHash = source.value("blockHash").toString("invalid");
        fBlockNumber = Helpers::toQUInt64(source.value("blockNumber"));
        fTransactionIndex = Helpers::toQUInt64(source.value("transactionIndex"));
        fValue = Helpers::toDecStrEther(source.value("value"));
        fGas = Helpers::toDecStr(source.value("gas"));
        fGasPrice = Helpers::toDecStrEther(source.value("gasPrice"));
        fInput = source.value("input").toString("invalid");

        lookupAccountAliases();
    }

    void TransactionInfo::lookupAccountAliases() {
        const QSettings settings;

        if ( settings.contains("alias/" + fSender) ) {
            fSenderAlias = settings.value("alias/" + fSender, QString()).toString();
        }

        if ( settings.contains("alias/" + fReceiver) ) {
            fReceiverAlias = settings.value("alias/" + fReceiver, QString()).toString();
        }
    }

    const QJsonObject TransactionInfo::toJson(bool decimal) const {
        QJsonObject result;
        result["hash"] = fHash;
        result["from"] = fSender;
        result["to"] = fReceiver;
        result["blockHash"] = fBlockHash;
        result["input"] = fInput;

        if ( decimal ) {
            result["value"] = fValue;
            result["gas"] = fGas;
            result["gasPrice"] = fGasPrice;
            result["blockNumber"] = (qint64) fBlockNumber;
            result["transactionIndex"] = (qint64) fTransactionIndex;
            result["nonce"] = (qint64) fNonce;
        } else { // hex
            result["value"] = Helpers::toHexWeiStr(fValue);
            result["gas"] = Helpers::decStrToHexStr(fGas);
            result["gasPrice"] = Helpers::toHexWeiStr(fGasPrice);
            result["blockNumber"] = Helpers::toHexStr(fBlockNumber);
            result["transactionIndex"] = Helpers::toHexStr(fTransactionIndex);
            result["nonce"] = Helpers::toHexStr(fNonce);
        }

        return result;
    }

    const QString TransactionInfo::toJsonString(bool decimal) const {
        const QJsonDocument doc(toJson(decimal));
        return doc.toJson();
    }


// ***************************** Helpers ***************************** //

    const QString Helpers::toDecStr(const QJsonValue& jv) {
        std::string hexStr = jv.toString("0x0").remove(0, 2).toStdString();
        const BigInt::Vin bv(hexStr, 16);
        QString decStr = QString(bv.toStrDec().data());

        return decStr;
    }

    const QString Helpers::toDecStrEther(const QJsonValue& jv) {
        QString decStr = toDecStr(jv);

        int dsl = decStr.length();
        if ( dsl <= 18 ) {
            decStr.prepend(QString(19 - dsl, '0'));
            dsl = decStr.length();
        }
        decStr.insert(dsl - 18, '.');
        return decStr;
    }

    const QString Helpers::toDecStr(quint64 val) {
        BigInt::Vin vinVal(val);
        return QString(vinVal.toStrDec().data());
    }

    const QString Helpers::toHexStr(quint64 val) {
        BigInt::Vin vinVal(val);
        return QString(vinVal.toStr0xHex().data());
    }

    const QString Helpers::toHexWeiStr(const QString& val) {
        QString decStr = val;

        int diff = 18;
        int n = decStr.indexOf('.');
        if ( n >= 0 ) {
            decStr.replace(".", "");
            diff = 18 - (decStr.length() - n);
        }

        for ( int i = 0; i < diff; i++ ) {
            decStr.append('0');
        }

        BigInt::Vin vinVal(decStr.toUtf8().data(), 10);
        QString res = QString(vinVal.toStr0xHex().data());

        return QString(vinVal.toStr0xHex().data());
    }

    const QString Helpers::toHexWeiStr(quint64 val) {
        BigInt::Vin vinVal(val);
        return QString(vinVal.toStr0xHex().data());
    }

    const QString Helpers::decStrToHexStr(const QString &dec) {
        BigInt::Vin vinVal(dec.toStdString(), 10);
        return QString(vinVal.toStrDec().data());
    }

    const QString Helpers::weiStrToEtherStr(const QString& wei) {
        QString weiStr = wei;
        while ( weiStr.length() < 18 ) {
            weiStr.insert(0, '0');
        }

        weiStr.insert(weiStr.length() - 18, '.');
        if ( weiStr.at(0) == '.' ) {
            weiStr.insert(0, '0');
        }
        return weiStr;
    }

    BigInt::Rossi Helpers::decStrToRossi(const QString& dec) {
        return BigInt::Rossi(dec.toStdString(), 10);
    }

    BigInt::Rossi Helpers::etherStrToRossi(const QString& dec) {
        QString decStr = dec;

        int diff = 18;
        int n = decStr.indexOf('.');
        if ( n >= 0 ) {
            decStr.replace(".", "");
            diff = 18 - (decStr.length() - n);
        }

        for ( int i = 0; i < diff; i++ ) {
            decStr.append('0');
        }

        return decStrToRossi(decStr);
    }

    const QString Helpers::formatEtherStr(const QString& ether) {
        QString decStr = ether;

        int n = decStr.indexOf('.');
        int diff;
        if ( n < 0 ) {
            decStr.append('.');
            n = decStr.indexOf('.');
            diff = 18;
        } else {
            diff = 18 - (decStr.length() - n - 1);
        }

        for ( int i = 0; i < diff; i++ ) {
            decStr.append('0');
        }

        return decStr;
    }

    const QJsonArray Helpers::toQJsonArray(const AccountList& list) {
        QJsonArray result;

        foreach ( const AccountInfo info, list ) {
            const QString hash = info.value(HashRole).toString();
            result.append(hash);
        }

        return result;
    }

    quint64 Helpers::toQUInt64(const QJsonValue& jv) {
        std::string hexStr = jv.toString("0x0").remove(0, 2).toStdString();
        BigInt::Vin vin(hexStr, 16);
        return vin.toUlong();
    }

}

