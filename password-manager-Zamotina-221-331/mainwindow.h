#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPushButton;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *titleLabel;
    QLabel *subtitleLabel;

    QPushButton *createVaultButton;
    QPushButton *openVaultButton;
};

#endif // MAINWINDOW_H
