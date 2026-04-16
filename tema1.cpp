#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#pragma comment(lib, "Advapi32.lib")

std::wstring GetTypeName(DWORD type)
{
    switch (type)
    {
    case REG_SZ: return L"REG_SZ";
    case REG_EXPAND_SZ: return L"REG_EXPAND_SZ";
    case REG_MULTI_SZ: return L"REG_MULTI_SZ";
    case REG_DWORD: return L"REG_DWORD";
    case REG_QWORD: return L"REG_QWORD";
    case REG_BINARY: return L"REG_BINARY";
    default: return L"ALT TIP";
    }
}

void PrintBinary(const BYTE* data, DWORD size)
{
    for (DWORD i = 0; i < size; i++)
    {
        std::wcout << std::hex << std::setw(2) << std::setfill(L'0')
            << static_cast<int>(data[i]) << L' ';
    }
    std::wcout << std::dec << std::endl;
}

void PrintMultiSz(const wchar_t* data, DWORD sizeBytes)
{
    const wchar_t* p = data;
    const wchar_t* end = reinterpret_cast<const wchar_t*>(
        reinterpret_cast<const BYTE*>(data) + sizeBytes
        );

    while (p < end && *p != L'\0')
    {
        std::wcout << L"    - " << p << std::endl;
        p += wcslen(p) + 1;
    }
}

void PrintValueData(DWORD type, const BYTE* data, DWORD dataSize)
{
    switch (type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    {
        const wchar_t* text = reinterpret_cast<const wchar_t*>(data);
        std::wcout << text << std::endl;
        break;
    }

    case REG_DWORD:
    {
        if (dataSize >= sizeof(DWORD))
        {
            DWORD value = *reinterpret_cast<const DWORD*>(data);
            std::wcout << value << L" (0x" << std::hex << value << std::dec << L")" << std::endl;
        }
        break;
    }

    case REG_QWORD:
    {
        if (dataSize >= sizeof(ULONGLONG))
        {
            ULONGLONG value = *reinterpret_cast<const ULONGLONG*>(data);
            std::wcout << value << L" (0x" << std::hex << value << std::dec << L")" << std::endl;
        }
        break;
    }

    case REG_MULTI_SZ:
    {
        PrintMultiSz(reinterpret_cast<const wchar_t*>(data), dataSize);
        break;
    }

    case REG_BINARY:
    default:
    {
        PrintBinary(data, dataSize);
        break;
    }
    }
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 3)
    {
        std::wcout << L"Utilizare:\n";
        std::wcout << L"  program.exe <radacina> <subcheie>\n\n";
        std::wcout << L"Exemple:\n";
        std::wcout << L"  program.exe HKCU Software\\Microsoft\\Windows\\CurrentVersion\\Run\n";
        std::wcout << L"  program.exe HKLM SOFTWARE\\Microsoft\\Windows\\CurrentVersion\n";
        return 1;
    }

    HKEY hRoot = nullptr;
    std::wstring rootName = argv[1];

    if (rootName == L"HKCU")
        hRoot = HKEY_CURRENT_USER;
    else if (rootName == L"HKLM")
        hRoot = HKEY_LOCAL_MACHINE;
    else if (rootName == L"HKCR")
        hRoot = HKEY_CLASSES_ROOT;
    else if (rootName == L"HKU")
        hRoot = HKEY_USERS;
    else if (rootName == L"HKCC")
        hRoot = HKEY_CURRENT_CONFIG;
    else
    {
        std::wcerr << L"Radacina invalida. Foloseste: HKCU, HKLM, HKCR, HKU sau HKCC.\n";
        return 1;
    }

    HKEY hKey;
    LONG result = RegOpenKeyExW(
        hRoot,
        argv[2],
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS)
    {
        std::wcerr << L"Eroare la deschiderea subcheii. Cod eroare: " << result << std::endl;
        return 1;
    }

    DWORD valueCount = 0;
    DWORD maxValueNameLen = 0;
    DWORD maxValueDataLen = 0;

    result = RegQueryInfoKeyW(
        hKey,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valueCount,
        &maxValueNameLen,
        &maxValueDataLen,
        nullptr,
        nullptr
    );

    if (result != ERROR_SUCCESS)
    {
        std::wcerr << L"Eroare la citirea informatiilor despre cheie. Cod eroare: " << result << std::endl;
        RegCloseKey(hKey);
        return 1;
    }

    std::wcout << L"Numar valori: " << valueCount << std::endl;
    std::wcout << L"----------------------------------------\n";

    std::vector<wchar_t> valueName(maxValueNameLen + 1);
    std::vector<BYTE> valueData(maxValueDataLen);

    for (DWORD index = 0; index < valueCount; index++)
    {
        DWORD valueNameLen = maxValueNameLen + 1;
        DWORD valueDataLen = maxValueDataLen;
        DWORD valueType = 0;

        result = RegEnumValueW(
            hKey,
            index,
            valueName.data(),
            &valueNameLen,
            nullptr,
            &valueType,
            valueData.data(),
            &valueDataLen
        );

        if (result == ERROR_SUCCESS)
        {
            std::wcout << L"Nume valoare : "
                << (valueNameLen > 0 ? valueName.data() : L"(Default)")
                << std::endl;

            std::wcout << L"Tip         : " << GetTypeName(valueType) << std::endl;
            std::wcout << L"Date        : ";
            PrintValueData(valueType, valueData.data(), valueDataLen);
            std::wcout << L"----------------------------------------\n";
        }
        else
        {
            std::wcerr << L"Eroare la enumerarea valorii cu index " << index
                << L". Cod eroare: " << result << std::endl;
        }
    }

    RegCloseKey(hKey);
    return 0;
}