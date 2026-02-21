#ifndef VAULTWINDOW_H
#define VAULTWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QTableView;
class QPushButton;
class QSortFilterProxyModel;
class CredentialsModel;

class VaultWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit VaultWindow(const QString &pin, QWidget *parent = nullptr);

private slots:
    void onFilterChanged(const QString &text);
    void onToggleReveal();

private:
    void load();
    void save();
    int selectedSourceRow() const;

private:
    QLineEdit *filterEdit{};
    QTableView *table{};
    QPushButton *revealBtn{};

    CredentialsModel *model{};
    QSortFilterProxyModel *proxy{};
    QString m_pin;
};

#endif // VAULTWINDOW_H
