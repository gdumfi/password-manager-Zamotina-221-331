#include "vaultwindow.h"
#include <QLabel>

VaultWindow::VaultWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Password Manager — Vault");
    resize(900, 600);

    auto *label = new QLabel("Окно учётных данных (следующий шаг)", this);
    label->setAlignment(Qt::AlignCenter);
    setCentralWidget(label);
}
