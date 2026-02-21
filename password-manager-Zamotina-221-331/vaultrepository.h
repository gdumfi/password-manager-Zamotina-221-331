#ifndef VAULTREPOSITORY_H
#define VAULTREPOSITORY_H

#include "credential.h"
#include <QVector>
#include <QString>

class VaultRepository
{
public:
    static QString defaultFilePath();

    static QVector<Credential> loadEncrypted(const QString &pin, QString *errorOut = nullptr);
    static bool saveEncrypted(const QVector<Credential> &creds, const QString &pin, QString *errorOut = nullptr);

    static bool tryUnlock(const QString &pin, QString *errorOut = nullptr);

    static bool decryptSecretFromB64(const QString &pin, const QString &secretB64,
                                     QString *loginOut, QString *passwordOut,
                                     QString *errorOut = nullptr);

    static bool encryptSecretToB64(const QString &pin, const QString &login, const QString &password,
                                   QString *secretB64Out, QString *errorOut = nullptr);

private:
    static QByteArray pinToKey(const QString &pin);
    static QByteArray iv();

    static bool aesEncrypt(const QByteArray &plain, const QByteArray &key, const QByteArray &iv,
                           QByteArray *cipherOut, QString *errorOut);

    static bool aesDecrypt(const QByteArray &cipher, const QByteArray &key, const QByteArray &iv,
                           QByteArray *plainOut, QString *errorOut);
};

#endif // VAULTREPOSITORY_H
