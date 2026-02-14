#include "credentialsmodel.h"

CredentialsModel::CredentialsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void CredentialsModel::setCredentials(QVector<Credential> creds)
{
    beginResetModel();
    m_creds = std::move(creds);
    //m_revealRow = -1;
    endResetModel();
}

void CredentialsModel::setRevealRow(int row)
{
    if (row == m_revealRow) return;

    const int old = m_revealRow;
    m_revealRow = row;

    // перерисуем старую и новую строки (если были)
    if (old >= 0 && old < m_creds.size())
        emit dataChanged(index(old, 0), index(old, 2));
    if (m_revealRow >= 0 && m_revealRow < m_creds.size())
        emit dataChanged(index(m_revealRow, 0), index(m_revealRow, 2));
}

int CredentialsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_creds.size();
}

int CredentialsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 3;
}

QVariant CredentialsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return {};
    const int r = index.row();
    const int c = index.column();
    if (r < 0 || r >= m_creds.size()) return {};

    const Credential &cred = m_creds[r];

    if (role == Qt::DisplayRole) {
        if (c == 0) return cred.url;

        const bool reveal = (r == m_revealRow);
        if (c == 1) return reveal ? cred.login : mask();
        if (c == 2) return reveal ? cred.password : mask();
    }

    return {};
}

QVariant CredentialsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (orientation != Qt::Horizontal) return {};

    switch (section) {
    case 0: return "URL";
    case 1: return "Логин";
    case 2: return "Пароль";
    default: return {};
    }
}
