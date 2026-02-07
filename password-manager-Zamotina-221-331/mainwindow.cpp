#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Password Manager");
    resize(900, 600);

    // Центральный виджет
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Основной layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setSpacing(20);

    // Заголовок
    titleLabel = new QLabel("Password Manager");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 26px;"
        "font-weight: bold;"
        );

    // Подзаголовок
    subtitleLabel = new QLabel("Создайте или откройте хранилище паролей");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "color: gray;"
        "font-size: 14px;"
        );

    // Кнопки
    createVaultButton = new QPushButton("Создать хранилище");
    openVaultButton   = new QPushButton("Открыть хранилище");

    createVaultButton->setMinimumHeight(45);
    openVaultButton->setMinimumHeight(45);

    createVaultButton->setStyleSheet(
        "font-size: 16px;"
        );
    openVaultButton->setStyleSheet(
        "font-size: 16px;"
        );

    // Сборка интерфейса
    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(30);
    layout->addWidget(createVaultButton);
    layout->addWidget(openVaultButton);
    layout->addStretch();
}
