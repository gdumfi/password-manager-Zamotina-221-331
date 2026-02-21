#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onLoginClicked();

private:
    void setLockedState(bool locked, const QString &message);

private:
    QLineEdit *pinEdit{};
    QPushButton *loginButton{};
    QLabel *infoLabel{};
};

#endif // MAINWINDOW_H
