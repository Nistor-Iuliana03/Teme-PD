#include <windows.h>
#include <iostream>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "User32.lib")

#define SERVICE_NAME L"HelloWorldService"

SERVICE_STATUS          g_ServiceStatus = {};
SERVICE_STATUS_HANDLE   g_StatusHandle = nullptr;
HANDLE                  g_StopEvent = nullptr;
HANDLE                  g_MessageThread = nullptr;

void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint)
{
    static DWORD checkPoint = 1;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = currentState;
    g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
    g_ServiceStatus.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING)
    {
        g_ServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED)
    {
        g_ServiceStatus.dwCheckPoint = 0;
    }
    else
    {
        g_ServiceStatus.dwCheckPoint = checkPoint++;
    }

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

DWORD WINAPI ShowHelloThread(LPVOID)
{
    MessageBoxW(
        nullptr,
        L"Hello World!",
        L"HelloWorldService",
        MB_OK | MB_ICONINFORMATION | MB_SERVICE_NOTIFICATION
    );
    return 0;
}

DWORD WINAPI ServiceCtrlHandlerEx(DWORD control, DWORD, LPVOID, LPVOID)
{
    switch (control)
    {
    case SERVICE_CONTROL_STOP:
        ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        if (g_StopEvent)
            SetEvent(g_StopEvent);

        return NO_ERROR;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

void WINAPI ServiceMain(DWORD, LPWSTR*)
{
    g_StatusHandle = RegisterServiceCtrlHandlerExW(
        SERVICE_NAME,
        ServiceCtrlHandlerEx,
        nullptr
    );

    if (!g_StatusHandle)
        return;

    ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    g_StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_StopEvent)
    {
        ReportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // Serviciul este gata
    ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // Mesajul se afișează din thread separat, după SERVICE_RUNNING
    g_MessageThread = CreateThread(nullptr, 0, ShowHelloThread, nullptr, 0, nullptr);

    // Așteaptă până când serviciul primește Stop
    WaitForSingleObject(g_StopEvent, INFINITE);

    if (g_MessageThread)
    {
        WaitForSingleObject(g_MessageThread, 5000);
        CloseHandle(g_MessageThread);
        g_MessageThread = nullptr;
    }

    if (g_StopEvent)
    {
        CloseHandle(g_StopEvent);
        g_StopEvent = nullptr;
    }

    ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

int wmain()
{
    SERVICE_TABLE_ENTRYW serviceTable[] =
    {
        { const_cast<LPWSTR>(SERVICE_NAME), ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable))
    {
        std::wcerr << L"Programul trebuie pornit de Service Control Manager.\n";
        std::wcerr << L"Cod eroare: " << GetLastError() << L"\n";
        return 1;
    }

    return 0;
}