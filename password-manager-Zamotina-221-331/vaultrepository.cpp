#include "vaultrepository.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QChar>
#include <openssl/evp.h>

QString VaultRepository::defaultFilePath()
{
    const QString exePath = QDir(QCoreApplication::applicationDirPath())
    .filePath("credentials.json");
    if (QFileInfo::exists(exePath)) {
        return exePath;
    }
    return QDir::current().filePath("credentials.json");
}

// IV
QByteArray VaultRepository::iv()
{
    return QByteArray::fromHex("00010203040506070809101112131415"); // 16
}

// key
QByteArray VaultRepository::pinToKey(const QString &pin)
{
    return QCryptographicHash::hash(pin.toUtf8(), QCryptographicHash::Sha256); // 32
}

bool VaultRepository::aesEncrypt(const QByteArray &plain, const QByteArray &key, const QByteArray &iv,QByteArray *cipherOut, QString *errorOut)
{


    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    QByteArray cipher;
    cipher.resize(plain.size() + 16);

    int outLen1 = 0, outLen2 = 0;

    bool ok =
        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) == 1
        && EVP_EncryptUpdate(ctx,
                             reinterpret_cast<unsigned char*>(cipher.data()), &outLen1,
                             reinterpret_cast<const unsigned char*>(plain.constData()), plain.size()) == 1
        && EVP_EncryptFinal_ex(ctx,
                               reinterpret_cast<unsigned char*>(cipher.data()) + outLen1, &outLen2) == 1;

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        if (errorOut) *errorOut = "OpenSSL: AES Encrypt failed";
        return false;
    }

    cipher.resize(outLen1 + outLen2);
    *cipherOut = cipher;
    return true;
}

bool VaultRepository::aesDecrypt(const QByteArray &cipher, const QByteArray &key, const QByteArray &iv,
                                 QByteArray *plainOut, QString *errorOut)
{

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (errorOut) *errorOut = "OpenSSL: EVP_CIPHER_CTX_new failed";
        return false;
    }

    QByteArray plain;
    plain.resize(cipher.size());

    int outLen1 = 0, outLen2 = 0;

    bool ok =
        EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) == 1
        && EVP_DecryptUpdate(ctx,
                             reinterpret_cast<unsigned char*>(plain.data()), &outLen1,
                             reinterpret_cast<const unsigned char*>(cipher.constData()), cipher.size()) == 1
        && EVP_DecryptFinal_ex(ctx,
                               reinterpret_cast<unsigned char*>(plain.data()) + outLen1, &outLen2) == 1;

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        if (errorOut) *errorOut = "Неверный PIN или повреждённые данные (AES Decrypt failed)";
        return false;
    }

    plain.resize(outLen1 + outLen2);
    *plainOut = plain;
    return true;
}

bool VaultRepository::tryUnlock(const QString &pin, QString *errorOut)
{
    QFile f(defaultFilePath());
    if (!f.exists()) {
        return true;
    }

    QString err;
    auto creds = loadEncrypted(pin, &err);
    Q_UNUSED(creds);

    if (!err.isEmpty()) {
        if (errorOut) *errorOut = err;
        return false;
    }
    return true;
}

// 2-й слой
QVector<Credential> VaultRepository::loadEncrypted(const QString &pin, QString *errorOut)
{
    QVector<Credential> out;

    QFile f(defaultFilePath());
    if (!f.exists()) return out;

    if (!f.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = "Не удалось открыть файл хранилища";
        return out;
    }

    const QByteArray fileCipher = f.readAll();
    f.close();

    const QByteArray key = pinToKey(pin);
    const QByteArray ivv = iv();

    QByteArray jsonPlain;
    if (!aesDecrypt(fileCipher, key, ivv, &jsonPlain, errorOut)) {
        return out;
    }

    QJsonParseError perr{};
    QJsonDocument doc = QJsonDocument::fromJson(jsonPlain, &perr);
    const QJsonObject root = doc.object();
    const QJsonValue credsVal = root.value("creds");
    const QJsonArray arr = credsVal.toArray();
    out.reserve(arr.size());

    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();

        Credential c;
        c.url = o.value("url").toString();
        c.secretHex = o.value("secret").toString(); // 1-й слой: hex

        if (!c.url.isEmpty())
            out.push_back(std::move(c));
    }

    return out;
}

bool VaultRepository::saveEncrypted(const QVector<Credential> &creds, const QString &pin, QString *errorOut)
{
    QJsonArray arr;

    for (const Credential &c : creds) {
        QJsonObject o;
        o["url"] = c.url;
        o["secret"] = c.secretHex;
        arr.push_back(o);
    }

    QJsonObject root;
    root["creds"] = arr;

    const QByteArray jsonPlain = QJsonDocument(root).toJson(QJsonDocument::Indented);

    const QByteArray key = pinToKey(pin);
    const QByteArray ivv = iv();

    QByteArray fileCipher;
    if (!aesEncrypt(jsonPlain, key, ivv, &fileCipher, errorOut)) {
        return false;
    }

    QFile f(defaultFilePath());

    f.write(fileCipher);
    f.close();
    return true;
}

// 1-й слой
bool VaultRepository::encryptSecretToHex(const QString &pin, const QString &login, const QString &password,QString *secretHexOut, QString *errorOut)
{
    const QByteArray key = pinToKey(pin);
    const QByteArray ivv = iv();

    QJsonObject s;
    s["login"] = login;
    s["password"] = password;

    const QByteArray plain = QJsonDocument(s).toJson(QJsonDocument::Compact);

    QByteArray cipher;
    if (!aesEncrypt(plain, key, ivv, &cipher, errorOut))
        return false;

    *secretHexOut = QString::fromLatin1(cipher.toHex());
    return true;
}

bool VaultRepository::decryptSecretFromHex(const QString &pin, const QString &secretHex, QString *loginOut, QString *passwordOut,QString *errorOut)
{
    const QByteArray key = pinToKey(pin);
    const QByteArray ivv = iv();

    const QByteArray cipher = QByteArray::fromHex(secretHex.toLatin1());

    QByteArray plain;
    if (!aesDecrypt(cipher, key, ivv, &plain, errorOut))
        return false;

    QJsonParseError perr{};
    QJsonDocument doc = QJsonDocument::fromJson(plain, &perr);

    const QJsonObject o = doc.object();
    *loginOut = o.value("login").toString();
    *passwordOut = o.value("password").toString();
    return true;
}
