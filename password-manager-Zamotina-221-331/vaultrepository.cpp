#include "vaultrepository.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
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

// IV фиксированный по заданию
QByteArray VaultRepository::iv()
{
    return QByteArray::fromHex("00010203040506070809101112131415"); // 16 bytes
}

// key = SHA256(pin)
QByteArray VaultRepository::pinToKey(const QString &pin)
{
    return QCryptographicHash::hash(pin.toUtf8(), QCryptographicHash::Sha256); // 32 bytes
}

bool VaultRepository::aesEncrypt(const QByteArray &plain, const QByteArray &key, const QByteArray &iv,
                                 QByteArray *cipherOut, QString *errorOut)
{
    if (key.size() != 32 || iv.size() != 16) {
        if (errorOut) *errorOut = "Некорректный ключ/IV";
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (errorOut) *errorOut = "OpenSSL: EVP_CIPHER_CTX_new failed";
        return false;
    }

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
    if (key.size() != 32 || iv.size() != 16) {
        if (errorOut) *errorOut = "Некорректный ключ/IV";
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (errorOut) *errorOut = "OpenSSL: EVP_CIPHER_CTX_new failed";
        return false;
    }

    QByteArray plain;
    plain.resize(cipher.size()); // будет <= cipher.size()

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

// Валидность PIN: смогли расшифровать файл и распарсить JSON
bool VaultRepository::tryUnlock(const QString &pin, QString *errorOut)
{
    QFile f(defaultFilePath());
    if (!f.exists()) {
        // первый запуск: файла нет — считаем PIN допустимым
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

// 2-й слой: весь файл зашифрован
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
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) *errorOut = "Расшифровка прошла, но JSON некорректен (возможно неверный PIN): " + perr.errorString();
        return out;
    }

    const QJsonObject root = doc.object();
    const QJsonValue credsVal = root.value("creds");
    if (!credsVal.isArray()) {
        if (errorOut) *errorOut = "Некорректный формат: поле 'creds' должно быть массивом";
        return out;
    }

    const QJsonArray arr = credsVal.toArray();
    out.reserve(arr.size());

    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();

        Credential c;
        c.url = o.value("url").toString();
        c.secretB64 = o.value("secret").toString(); // 1-й слой: base64
        // login/password пустые до раскрытия

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
        o["secret"] = c.secretB64;
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
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut) *errorOut = "Не удалось записать файл хранилища";
        return false;
    }

    f.write(fileCipher);
    f.close();
    return true;
}

// 1-й слой: secret = AES(JSON{login,password}) -> base64
bool VaultRepository::encryptSecretToB64(const QString &pin, const QString &login, const QString &password,
                                         QString *secretB64Out, QString *errorOut)
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

    *secretB64Out = QString::fromLatin1(cipher.toBase64());
    return true;
}

bool VaultRepository::decryptSecretFromB64(const QString &pin, const QString &secretHexSpaced,
                                           QString *loginOut, QString *passwordOut,
                                           QString *errorOut)
{
    const QByteArray key = pinToKey(pin);
    const QByteArray ivv = iv();

    // secret хранится как "4c d5 be ..." -> убираем пробелы/переводы строк
    QString hex = secretHexSpaced;
    hex.remove('\r');
    hex.remove('\n');
    hex.remove(' ');
    hex = hex.trimmed();

    // минимальная валидация: длина должна быть чётной и хотя бы 32 байта (64 hex)
    if (hex.isEmpty() || (hex.size() % 2) != 0) {
        if (errorOut) *errorOut = "Secret: некорректный hex (пустой или нечётная длина)";
        return false;
    }

    const QByteArray cipher = QByteArray::fromHex(hex.toLatin1());
    if (cipher.isEmpty()) {
        if (errorOut) *errorOut = "Secret: не удалось декодировать hex";
        return false;
    }

    QByteArray plain;
    if (!aesDecrypt(cipher, key, ivv, &plain, errorOut))
        return false;

    QJsonParseError perr{};
    QJsonDocument doc = QJsonDocument::fromJson(plain, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) *errorOut = "Secret расшифрован, но JSON внутри некорректен: " + perr.errorString();
        return false;
    }

    const QJsonObject o = doc.object();
    *loginOut = o.value("login").toString();
    *passwordOut = o.value("password").toString();
    return true;
}
