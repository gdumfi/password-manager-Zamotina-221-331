#include <QApplication>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>

#include "mainwindow.h"

#ifdef _WIN32
#include <windows.h>
#include <winnt.h>
#endif

bool checkForDebugger()
{
#ifdef _WIN32
    if (IsDebuggerPresent()) {
        QMessageBox::critical(nullptr, "Ошибка безопасности",
                              "Обнаружен отладчик. Программа не может быть запущена.");
        return true;
    }
#endif
    return false;
}

bool verifyTextSectionSha256(QString *errorOut = nullptr)
{
#ifdef _WIN32
    // Получаем путь к запущенному EXE
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
        if (errorOut) *errorOut = "Не удалось получить путь к исполняемому файлу";
        return false;
    }

    // Читаем EXE с диска
    QFile file(QString::fromWCharArray(exePath));
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = "Не удалось открыть исполняемый файл";
        return false;
    }
    const QByteArray fileData = file.readAll();
    file.close();

    const auto *base = reinterpret_cast<const unsigned char*>(fileData.constData());

    // Читаем заголовки DOS и NT
    auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        if (errorOut) *errorOut = "Некорректная DOS-сигнатура";
        return false;
    }

    auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        if (errorOut) *errorOut = "Некорректная NT-сигнатура";
        return false;
    }

    // Ищем секцию .text
    const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    const WORD secCount = nt->FileHeader.NumberOfSections;
    const IMAGE_SECTION_HEADER *textSec = nullptr;

    for (WORD i = 0; i < secCount; ++i) {
        char name[9]{};
        memcpy(name, sec[i].Name, 8);
        if (strcmp(name, ".text") == 0) {
            textSec = &sec[i];
            break;
        }
    }

    if (!textSec) {
        if (errorOut) *errorOut = "Не найдена секция .text";
        return false;
    }

    // Виртуальный адрес начала .text и размер
    const DWORD virtualAddress = textSec->VirtualAddress;
    const DWORD virtualSize    = textSec->Misc.VirtualSize;

    // Смещение и размер в файле (используем для стабильного хеширования)
    const DWORD rawOffset = textSec->PointerToRawData;
    const DWORD rawSize   = textSec->SizeOfRawData;

    if (rawOffset + rawSize > static_cast<DWORD>(fileData.size())) {
        if (errorOut) *errorOut = "Некорректные данные секции .text в файле";
        return false;
    }

    qDebug() << "Виртуальный адрес .text: 0x" + QByteArray::number(virtualAddress, 16).toUpper();
    qDebug() << "Размер .text (VirtualSize):" << virtualSize << "байт";

    // Вычисляем SHA-256 секции .text из файла
    const QByteArray textData(fileData.constData() + rawOffset, rawSize);
    const QByteArray calculatedHash = QCryptographicHash::hash(textData, QCryptographicHash::Sha256);

    qDebug() << "Calculated SHA256:" << calculatedHash.toHex();

    // Эталонный хеш
    const QByteArray referenceHash = QByteArray::fromHex(
        "3d093566cee56282e3031bbe2bb549ef614ab8b49b236793be85d7961d389ce8"
        );

    if (calculatedHash != referenceHash) {
        if (errorOut) *errorOut = QString("Контрольная сумма не совпала!\n"
                                          "Ожидалось: %1\n"
                                          "Получено:  %2")
                                      .arg(QString(referenceHash.toHex()))
                                      .arg(QString(calculatedHash.toHex()));
        return false;
    }

    return true;
#else
    Q_UNUSED(errorOut);
    return true;
#endif
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Проверяем наличие отладчика
    if (checkForDebugger())
        return -1;

    // Проверяем контрольную сумму секции .text
    QString err;
    if (!verifyTextSectionSha256(&err)) {
        QMessageBox::critical(nullptr, "Ошибка", "Обнаружена модификация приложения: " + err);
        return -1;
    }

    MainWindow window;
    window.show();

    return app.exec();
}
