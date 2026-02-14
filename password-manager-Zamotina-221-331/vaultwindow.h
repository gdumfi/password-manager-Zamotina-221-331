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
    explicit VaultWindow(QWidget *parent = nullptr);

private slots:
    void onFilterChanged(const QString &text);
    void onToggleReveal();

private:
    void load();
    void save();
    int selectedSourceRow() const; // индекс выбранной строки в исходной модели

private:
    QLineEdit *filterEdit{};
    QTableView *table{};
    QPushButton *revealBtn{};

    CredentialsModel *model{};
    QSortFilterProxyModel *proxy{};
};

#endif // VAULTWINDOW_H
