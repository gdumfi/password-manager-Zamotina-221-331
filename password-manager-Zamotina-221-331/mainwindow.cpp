#include "mainwindow.h"
#include "vaultwindow.h"
#include "vaultrepository.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

// создаем окно логина
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Password Manager — Login");
    resize(520, 320);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto *centerBox = new QVBoxLayout();
    centerBox->setSpacing(10);

    auto *title = new QLabel("Разблокировка хранилища");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:18px;font-weight:600;");

    infoLabel = new QLabel("Введите PIN-код");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("color:gray;");

    pinEdit = new QLineEdit();
    pinEdit->setPlaceholderText("PIN-код");
    pinEdit->setEchoMode(QLineEdit::Password);
    pinEdit->setMaxLength(64);

    loginButton = new QPushButton("Войти");
    loginButton->setMinimumHeight(40);

    auto *row = new QHBoxLayout();
    row->setSpacing(10);
    row->addWidget(pinEdit, 1);
    row->addWidget(loginButton, 0);

    centerBox->addWidget(title);
    centerBox->addWidget(infoLabel);
    centerBox->addLayout(row);

    root->addStretch(1);
    root->addLayout(centerBox);
    root->addStretch(1);

    connect(loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(pinEdit, &QLineEdit::returnPressed, this, &MainWindow::onLoginClicked);
}

// блокируем ввод если надо
void MainWindow::setLockedState(bool locked, const QString &message)
{
    pinEdit->setEnabled(!locked);
    loginButton->setEnabled(!locked);
    infoLabel->setText(message);

    if (locked) {
        infoLabel->setStyleSheet("color:#b00020; font-weight:600;");
    } else {
        infoLabel->setStyleSheet("color:gray;");
    }
}

// проверяем pin расшифровкой файла и открываем хранилище
void MainWindow::onLoginClicked()
{
    const QString pin = pinEdit->text().trimmed();

    QString err;
    if (!VaultRepository::tryUnlock(pin, &err)) {
        setLockedState(false, err.isEmpty() ? "Неверный PIN-код" : err);
        pinEdit->selectAll();
        pinEdit->setFocus();
        return;
    }

    auto *vault = new VaultWindow(pin);
    vault->show();
    hide();
}
