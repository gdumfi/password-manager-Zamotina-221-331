#ifndef CREDENTIAL_H
#define CREDENTIAL_H

#include <QString>

struct Credential
{
    QString url;
    QString secretHex;
    QString login;
    QString password;
};

#endif // CREDENTIAL_H
