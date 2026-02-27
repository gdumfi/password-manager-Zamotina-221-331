#ifndef CREDENTIAL_H
#define CREDENTIAL_H

#include <QString>

struct Credential
{
    QString url;
    QString secretB64;
    QString login;
    QString password;
};

#endif // CREDENTIAL_H
