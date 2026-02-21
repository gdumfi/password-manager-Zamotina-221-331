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
    static constexpr const char* kMasterPin = "5986"; // PIN константой (по заданию)

private slots:
    void onLoginClicked();

private:
    bool isPinValid(const QString &pin) const;
    void setLockedState(bool locked, const QString &message);

private:
    QLineEdit *pinEdit{};
    QPushButton *loginButton{};
    QLabel *infoLabel{};


};

#endif // MAINWINDOW_H
