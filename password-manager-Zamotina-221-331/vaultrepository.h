#ifndef VAULTREPOSITORY_H
#define VAULTREPOSITORY_H

#include "credential.h"
#include <QVector>
#include <QString>

class VaultRepository
{
public:
    static QString defaultFilePath();

    static QVector<Credential> load(QString *errorOut = nullptr);
    static bool save(const QVector<Credential> &creds, QString *errorOut = nullptr);
};

#endif // VAULTREPOSITORY_H
