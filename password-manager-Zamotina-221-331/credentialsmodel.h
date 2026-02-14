#ifndef CREDENTIALSMODEL_H
#define CREDENTIALSMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "credential.h"

class CredentialsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit CredentialsModel(QObject *parent = nullptr);

    void setCredentials(QVector<Credential> creds);
    const QVector<Credential>& credentials() const { return m_creds; }
    QVector<Credential>& credentials() { return m_creds; }

    // показать/скрыть логин+пароль для одной строки
    void setRevealRow(int row); // -1 = скрыть всё
    int revealRow() const { return m_revealRow; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<Credential> m_creds;
    int m_revealRow = -1;

    static QString mask() { return "••••••••"; }
};

#endif // CREDENTIALSMODEL_H
