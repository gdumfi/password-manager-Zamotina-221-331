#include "mainwindow.h"
#include "vaultwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Password Manager — Login");
    resize(520, 320);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    // главный вертикальный layout
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    // внутренний блок по центру
    auto *centerBox = new QVBoxLayout();
    centerBox->setSpacing(10);

    auto *title = new QLabel("Разблокировка хранилища");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:18px;font-weight:600;");

    infoLabel = new QLabel("Введите PIN-код (мастер-пароль)");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("color:gray;");

    pinEdit = new QLineEdit();
    pinEdit->setPlaceholderText("PIN-код");
    pinEdit->setEchoMode(QLineEdit::Password); // маскировка
    pinEdit->setMaxLength(64);

    loginButton = new QPushButton("Войти");
    loginButton->setMinimumHeight(40);
    auto *toggleButton = new QPushButton("TEST NA BLOCK"); // для теста потом удали это
    toggleButton->setMinimumHeight(36);// для теста потом удали это

    // поле + кнопка
    auto *row = new QHBoxLayout();
    row->setSpacing(10);
    row->addWidget(pinEdit, 1);
    row->addWidget(loginButton, 0);

    centerBox->addWidget(title);
    centerBox->addWidget(infoLabel);
    centerBox->addLayout(row);
    centerBox->addWidget(toggleButton);// для теста потом удали это

    // центровка
    root->addStretch(1);
    root->addLayout(centerBox);
    root->addStretch(1);

    connect(loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(pinEdit, &QLineEdit::returnPressed, this, &MainWindow::onLoginClicked);
    connect(toggleButton, &QPushButton::clicked, this, [this]() {
        static bool locked = false;
        locked = !locked;

        if (locked)
            setLockedState(true, "ТЕСТ режим защиты ");
        else
            setLockedState(false, "ТЕСТ режим ввода ");
    });

}

bool MainWindow::isPinValid(const QString &pin) const
{
    return pin == QString::fromUtf8(kMasterPin);
}

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

void MainWindow::onLoginClicked()
{
    const QString pin = pinEdit->text();

    if (!isPinValid(pin)) {
        setLockedState(false, "Неверный PIN-код. Повторите ввод.");
        pinEdit->selectAll();
        pinEdit->setFocus();
        return;
    }

    // PIN верный переход на следующее окно
    auto *vault = new VaultWindow();
    vault->show();

    // текущее окно можно скрыть
    this->hide();
}

