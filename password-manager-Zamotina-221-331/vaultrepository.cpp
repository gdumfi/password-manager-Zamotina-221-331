#include "vaultrepository.h"

#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

QString VaultRepository::defaultFilePath()
{
    //const QString exeDir = QCoreApplication::applicationDirPath();
    //return QDir(exeDir).filePath("credentials.json");
    const QString exePath = QDir(QCoreApplication::applicationDirPath())
                                .filePath("credentials.json");
    if (QFileInfo::exists(exePath)) {
        return exePath;
    }

    return QDir::current().filePath("credentials.json");
}

QVector<Credential> VaultRepository::load(QString *errorOut)
{
    QVector<Credential> out;

    QFile f(defaultFilePath());
    if (!f.exists()) {
        // если файла нет — просто возвращаем пусто (UI всё равно работает)
        return out;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = "Не удалось открыть файл JSON";
        return out;
    }

    const QByteArray raw = f.readAll();
    f.close();

    QJsonParseError perr{};
    QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isArray()) {
        if (errorOut) *errorOut = "Ошибка парсинга JSON: " + perr.errorString();
        return out;
    }

    const QJsonArray arr = doc.array();
    out.reserve(arr.size());

    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();

        Credential c;
        c.url = o.value("url").toString();
        c.login = o.value("login").toString();
        c.password = o.value("password").toString();

        if (!c.url.isEmpty())
            out.push_back(c);
    }

    return out;
}

bool VaultRepository::save(const QVector<Credential> &creds, QString *errorOut)
{
    QJsonArray arr;

    for (const Credential &c : creds) {
        QJsonObject o;
        o["url"] = c.url;
        o["login"] = c.login;
        o["password"] = c.password;
        arr.push_back(o);
    }

    const QJsonDocument doc(arr);
    const QByteArray raw = doc.toJson(QJsonDocument::Indented);

    QFile f(defaultFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut) *errorOut = "Не удалось записать файл JSON";
        return false;
    }

    f.write(raw);
    f.close();
    return true;
}
