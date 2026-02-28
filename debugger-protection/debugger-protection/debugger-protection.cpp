#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cwchar>

// Полный путь к EXE менеджера паролей
static const wchar_t *DEFAULT_TARGET = L"C:\\Users\\akum1\\QTProject\\password-manager-Zamotina-221-331\\password-manager-Zamotina-221-331\\build\\Desktop_Qt_6_10_2_MinGW_64_bit-Debug\\password-manager-Zamotina-221-331.exe";

// Пути к Qt и MinGW DLL (нужны менеджеру паролей для запуска)
static const wchar_t *QT_BIN_DIR = L"C:\\Qt\\6.10.2\\mingw_64\\bin";
static const wchar_t *MINGW_BIN_DIR = L"C:\\Qt\\Tools\\mingw1630_64\\bin";

// Формирует блок окружения с добавленными путями Qt/MinGW в PATH
wchar_t *buildEnvironment()
{
    // Берём текущий PATH
    wchar_t oldPath[32768] = {};
    GetEnvironmentVariableW(L"PATH", oldPath, 32768);

    // Собираем новый PATH: Qt + MinGW + старый
    wchar_t newPath[32768] = {};
    swprintf_s(newPath, 32768, L"%s;%s;%s", QT_BIN_DIR, MINGW_BIN_DIR, oldPath);
    SetEnvironmentVariableW(L"PATH", newPath);

    // Формируем environment block (все переменные окружения подряд, каждая завершена \0, блок — \0\0)
    wchar_t *envBlock = GetEnvironmentStringsW();
    return envBlock; // вызывающий код должен вызвать FreeEnvironmentStringsW()
}

// Извлекает директорию из полного пути к файлу
void getDirectory(const wchar_t *filePath, wchar_t *dir, size_t dirSize)
{
    wcsncpy_s(dir, dirSize, filePath, _TRUNCATE);
    wchar_t *lastSlash = wcsrchr(dir, L'\\');
    if (lastSlash)
        *lastSlash = L'\0';
}

// Запускает целевой процесс с правильным окружением
bool launchTarget(const wchar_t *exePath, PROCESS_INFORMATION &pi)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    // Рабочая директория = папка с EXE менеджера паролей
    wchar_t workDir[MAX_PATH] = {};
    getDirectory(exePath, workDir, MAX_PATH);

    // Готовим окружение с Qt/MinGW в PATH
    wchar_t *envBlock = buildEnvironment();

    BOOL ok = CreateProcessW(
        exePath,          // путь к EXE
        nullptr,          // командная строка
        nullptr, nullptr, // безопасность
        FALSE,            // не наследовать дескрипторы
        CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
        envBlock, // окружение с Qt в PATH
        workDir,  // рабочая директория = папка с EXE
        &si, &pi);

    FreeEnvironmentStringsW(envBlock);

    if (!ok)
    {
        printf("[SATELLITE] CreateProcess FAILED, error = %lu\n", GetLastError());
        return false;
    }

    printf("[SATELLITE] Процесс запущен: PID = %lu\n", pi.dwProcessId);
    return true;
}

// Подключается к процессу как отладчик
bool attachDebugger(DWORD pid)
{
    if (!DebugActiveProcess(pid))
    {
        printf("[SATELLITE] DebugActiveProcess FAILED, error = %lu\n", GetLastError());
        return false;
    }

    printf("[SATELLITE] Подключён к процессу %lu как отладчик\n", pid);

    // Не убивать дочерний процесс при завершении спутника
    DebugSetProcessKillOnExit(FALSE);
    return true;
}

// Название события отладки (для лога)
const char *debugEventName(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_DEBUG_EVENT:
        return "EXCEPTION";
    case CREATE_THREAD_DEBUG_EVENT:
        return "CREATE_THREAD";
    case CREATE_PROCESS_DEBUG_EVENT:
        return "CREATE_PROCESS";
    case EXIT_THREAD_DEBUG_EVENT:
        return "EXIT_THREAD";
    case EXIT_PROCESS_DEBUG_EVENT:
        return "EXIT_PROCESS";
    case LOAD_DLL_DEBUG_EVENT:
        return "LOAD_DLL";
    case UNLOAD_DLL_DEBUG_EVENT:
        return "UNLOAD_DLL";
    case OUTPUT_DEBUG_STRING_EVENT:
        return "DEBUG_STRING";
    case RIP_EVENT:
        return "RIP";
    default:
        return "UNKNOWN";
    }
}

// Главный цикл обработки событий отладки
void debugLoop(DWORD pid)
{
    DEBUG_EVENT debugEvent{};
    bool running = true;

    printf("Вхожу в цикл обработки событий отладки...\n");

    while (running)
    {
        // Ждём событие отладки (таймаут 1000 мс)
        if (!WaitForDebugEvent(&debugEvent, 1000))
        {
            // Таймаут — проверяем, жив ли процесс
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
            if (!hProcess)
            {
                printf("Процесс завершён (больше не существует)\n");
                break;
            }

            DWORD exitCode = 0;
            GetExitCodeProcess(hProcess, &exitCode);
            CloseHandle(hProcess);

            if (exitCode != STILL_ACTIVE)
            {
                printf("Процесс завершён с кодом %lu\n", exitCode);
                break;
            }
            continue;
        }

        DWORD continueStatus = DBG_CONTINUE;

        printf("Событие: %-16s (PID=%lu, TID=%lu)\n",
               debugEventName(debugEvent.dwDebugEventCode),
               debugEvent.dwProcessId,
               debugEvent.dwThreadId);

        switch (debugEvent.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:
        {
            DWORD exCode = debugEvent.u.Exception.ExceptionRecord.ExceptionCode;

            // Системная точка останова при старте процесса — пропускаем
            if (exCode == EXCEPTION_BREAKPOINT)
            {
                printf("-> Точка останова (системная, пропускаем)\n");
                continueStatus = DBG_CONTINUE;
            }
            else
            {
                // Остальные исключения передаём обратно процессу
                printf("-> Исключение 0x%08lX, передаём процессу\n", exCode);
                continueStatus = DBG_EXCEPTION_NOT_HANDLED;
            }
            break;
        }

        case CREATE_PROCESS_DEBUG_EVENT:
            // Закрываем дескриптор файла образа (иначе утечка)
            if (debugEvent.u.CreateProcessInfo.hFile)
                CloseHandle(debugEvent.u.CreateProcessInfo.hFile);
            break;

        case LOAD_DLL_DEBUG_EVENT:
            // Закрываем дескриптор файла DLL
            if (debugEvent.u.LoadDll.hFile)
                CloseHandle(debugEvent.u.LoadDll.hFile);
            break;

        case EXIT_PROCESS_DEBUG_EVENT:
            printf("Целевой процесс завершился с кодом %lu\n",
                   debugEvent.u.ExitProcess.dwExitCode);
            running = false;
            break;

        default:
            break;
        }

        // Продолжаем выполнение целевого процесса
        ContinueDebugEvent(debugEvent.dwProcessId,
                           debugEvent.dwThreadId,
                           continueStatus);
    }

    printf("Цикл отладки завершён\n");
}

int wmain(int argc, wchar_t *argv[])
{
    // Путь к EXE можно передать первым аргументом
    const wchar_t *targetExe = (argc > 1) ? argv[1] : DEFAULT_TARGET;

    wprintf(L"Целевой EXE: %s\n", targetExe);

    // 1. Запускаем менеджер паролей
    PROCESS_INFORMATION pi{};
    if (!launchTarget(targetExe, pi))
    {
        return 1;
    }

    // Даём процессу время на инициализацию
    Sleep(500);

    // 2. Подключаемся как отладчик
    if (!attachDebugger(pi.dwProcessId))
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1;
    }

    // 3. Цикл обработки событий отладки
    debugLoop(pi.dwProcessId);

    // Отключаемся от процесса
    DebugActiveProcessStop(pi.dwProcessId);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("Спутник завершён\n");
    return 0;
}
