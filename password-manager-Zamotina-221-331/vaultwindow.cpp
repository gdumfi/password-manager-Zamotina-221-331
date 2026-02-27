#include "vaultwindow.h"
#include "credentialsmodel.h"
#include "vaultrepository.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableView>
#include <QPushButton>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>

VaultWindow::VaultWindow(const QString &pin, QWidget *parent)
    : QMainWindow(parent), m_pin(pin)
{
    setWindowTitle("Менеджер паролей");
    resize(900, 600);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    // Поиск (на всю ширину)
    filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText("Фильтр по URL (например: github, bank, mail...)");
    filterEdit->setClearButtonEnabled(true);

    // Таблица (занимает всё оставшееся)
    table = new QTableView();
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setStretchLastSection(true);

    model = new CredentialsModel(this);

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setFilterKeyColumn(0); // фильтруем по URL
    table->setModel(proxy);

    // кнопки
    revealBtn = new QPushButton("Показать");

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(8);
    buttons->addStretch(1);
    buttons->addWidget(revealBtn);

    // сборка UI
    root->addWidget(filterEdit);
    root->addWidget(table, 1);
    root->addLayout(buttons);

    // слоты
    connect(filterEdit, &QLineEdit::textChanged, this, &VaultWindow::onFilterChanged);
    connect(revealBtn, &QPushButton::clicked, this, &VaultWindow::onToggleReveal);

    load();
}

void VaultWindow::onFilterChanged(const QString &text)
{
    proxy->setFilterFixedString(text);
}

int VaultWindow::selectedSourceRow() const
{
    const QModelIndex proxyIndex = table->currentIndex();
    if (!proxyIndex.isValid())
        return -1;

    const QModelIndex srcIndex = proxy->mapToSource(proxyIndex);
    return srcIndex.row();
}

void VaultWindow::load()
{
    QString err;
    QVector<Credential> creds = VaultRepository::loadEncrypted(m_pin, &err);

    if (!err.isEmpty()) {
        QMessageBox::warning(this, "Загрузка", err);
    }

    model->setCredentials(std::move(creds));
}

void VaultWindow::save()
{
    QString err;
    if (!VaultRepository::saveEncrypted(model->credentials(), m_pin, &err)) {
        QMessageBox::warning(this, "Сохранение", err);
    }
}

// показываем логин и пароль после ввода pin
void VaultWindow::onToggleReveal()
{
    const int row = selectedSourceRow();
    if (row < 0) {
        QMessageBox::information(this, "Показать", "Выберите строку в таблице");
        return;
    }

    if (model->revealRow() == row) {
        model->setRevealRow(-1);
        revealBtn->setText("Показать");
        return;
    }

    bool ok = false;
    const QString pin = QInputDialog::getText(this,"PIN","Введите PIN для раскрытия",QLineEdit::Password,"",&ok).trimmed();

    if (!ok) return;

    auto &creds = model->credentials();
    if (row < 0 || row >= creds.size()) return;

    QString login, pass, err;
    if (!VaultRepository::decryptSecretFromB64(pin, creds[row].secretB64, &login, &pass, &err)) {
        QMessageBox::warning(this, "Ошибка", err.isEmpty() ? "Неверный PIN" : err);
        return;
    }

    creds[row].login = login;
    creds[row].password = pass;

    model->setRevealRow(row);
    revealBtn->setText("Скрыть");
}
