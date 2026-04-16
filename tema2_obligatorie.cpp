#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <devpropdef.h>
#pragma comment(lib, "SetupAPI.lib")
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <io.h>
#include <fcntl.h>
#include <initguid.h>


#pragma comment(lib, "SetupAPI.lib")
#pragma comment(lib, "Ole32.lib")

bool EqualPropKey(const DEVPROPKEY& a, const DEVPROPKEY& b)
{
    return a.pid == b.pid && IsEqualGUID(a.fmtid, b.fmtid);
}

std::wstring GuidToString(const GUID& guid)
{
    wchar_t buffer[64] = {};
    StringFromGUID2(guid, buffer, 64);
    return buffer;
}

std::wstring FileTimeToString(const FILETIME& ft)
{
    SYSTEMTIME stUTC{};
    if (!FileTimeToSystemTime(&ft, &stUTC))
        return L"(FILETIME invalid)";

    wchar_t buffer[64] = {};
    swprintf_s(buffer, L"%02u.%02u.%04u %02u:%02u:%02u UTC",
        stUTC.wDay, stUTC.wMonth, stUTC.wYear,
        stUTC.wHour, stUTC.wMinute, stUTC.wSecond);
    return buffer;
}

std::wstring BinaryToHex(const BYTE* data, DWORD size)
{
    std::wstringstream ss;
    DWORD limit = std::min<DWORD>(size, 64);

    for (DWORD i = 0; i < limit; ++i)
    {
        if (i) ss << L' ';
        ss << std::uppercase << std::hex << std::setw(2) << std::setfill(L'0')
            << static_cast<unsigned>(data[i]);
    }

    if (size > limit)
        ss << L" ...";

    return ss.str();
}

std::wstring MultiSzToString(const wchar_t* data, DWORD sizeBytes)
{
    std::wstringstream ss;
    const wchar_t* p = data;
    const wchar_t* end = reinterpret_cast<const wchar_t*>(
        reinterpret_cast<const BYTE*>(data) + sizeBytes);

    bool first = true;
    while (p < end && *p != L'\0')
    {
        if (!first) ss << L" | ";
        ss << p;
        p += wcslen(p) + 1;
        first = false;
    }

    return first ? L"(lista vida)" : ss.str();
}

std::wstring TypeToString(DEVPROPTYPE type)
{
    if (type == DEVPROP_TYPE_STRING_LIST) return L"DEVPROP_TYPE_STRING_LIST";
    if (type == DEVPROP_TYPE_BINARY) return L"DEVPROP_TYPE_BINARY";

    switch (type & DEVPROP_MASK_TYPE)
    {
    case DEVPROP_TYPE_EMPTY: return L"DEVPROP_TYPE_EMPTY";
    case DEVPROP_TYPE_NULL: return L"DEVPROP_TYPE_NULL";
    case DEVPROP_TYPE_SBYTE: return L"DEVPROP_TYPE_SBYTE";
    case DEVPROP_TYPE_BYTE: return L"DEVPROP_TYPE_BYTE";
    case DEVPROP_TYPE_INT16: return L"DEVPROP_TYPE_INT16";
    case DEVPROP_TYPE_UINT16: return L"DEVPROP_TYPE_UINT16";
    case DEVPROP_TYPE_INT32: return L"DEVPROP_TYPE_INT32";
    case DEVPROP_TYPE_UINT32: return L"DEVPROP_TYPE_UINT32";
    case DEVPROP_TYPE_INT64: return L"DEVPROP_TYPE_INT64";
    case DEVPROP_TYPE_UINT64: return L"DEVPROP_TYPE_UINT64";
    case DEVPROP_TYPE_FLOAT: return L"DEVPROP_TYPE_FLOAT";
    case DEVPROP_TYPE_DOUBLE: return L"DEVPROP_TYPE_DOUBLE";
    case DEVPROP_TYPE_GUID: return L"DEVPROP_TYPE_GUID";
    case DEVPROP_TYPE_FILETIME: return L"DEVPROP_TYPE_FILETIME";
    case DEVPROP_TYPE_BOOLEAN: return L"DEVPROP_TYPE_BOOLEAN";
    case DEVPROP_TYPE_STRING: return L"DEVPROP_TYPE_STRING";
    case DEVPROP_TYPE_DEVPROPKEY: return L"DEVPROP_TYPE_DEVPROPKEY";
    case DEVPROP_TYPE_DEVPROPTYPE: return L"DEVPROP_TYPE_DEVPROPTYPE";
    case DEVPROP_TYPE_ERROR: return L"DEVPROP_TYPE_ERROR";
    case DEVPROP_TYPE_NTSTATUS: return L"DEVPROP_TYPE_NTSTATUS";
    case DEVPROP_TYPE_STRING_INDIRECT: return L"DEVPROP_TYPE_STRING_INDIRECT";
    default:
        return L"TIP NECUNOSCUT";
    }
}

std::wstring ValueToString(DEVPROPTYPE type, const BYTE* data, DWORD size)
{
    if (type == DEVPROP_TYPE_EMPTY) return L"(empty)";
    if (type == DEVPROP_TYPE_NULL) return L"(null)";

    if (type == DEVPROP_TYPE_STRING_LIST)
    {
        return MultiSzToString(reinterpret_cast<const wchar_t*>(data), size);
    }

    if (type == DEVPROP_TYPE_BINARY)
    {
        return BinaryToHex(data, size);
    }

    std::wstringstream ss;

    switch (type & DEVPROP_MASK_TYPE)
    {
    case DEVPROP_TYPE_STRING:
    case DEVPROP_TYPE_STRING_INDIRECT:
        return reinterpret_cast<const wchar_t*>(data);

    case DEVPROP_TYPE_BOOLEAN:
        if (size >= sizeof(DEVPROP_BOOLEAN))
            return (*reinterpret_cast<const DEVPROP_BOOLEAN*>(data)) ? L"TRUE" : L"FALSE";
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_UINT32:
        if (size >= sizeof(ULONG))
        {
            ULONG v = *reinterpret_cast<const ULONG*>(data);
            ss << v << L" (0x" << std::hex << std::uppercase << v << L")";
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_INT32:
        if (size >= sizeof(LONG))
        {
            LONG v = *reinterpret_cast<const LONG*>(data);
            ss << v;
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_UINT64:
        if (size >= sizeof(ULONG64))
        {
            ULONG64 v = *reinterpret_cast<const ULONG64*>(data);
            ss << v << L" (0x" << std::hex << std::uppercase << v << L")";
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_INT64:
        if (size >= sizeof(LONGLONG))
        {
            LONGLONG v = *reinterpret_cast<const LONGLONG*>(data);
            ss << v;
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_UINT16:
        if (size >= sizeof(USHORT))
        {
            USHORT v = *reinterpret_cast<const USHORT*>(data);
            ss << v;
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_INT16:
        if (size >= sizeof(SHORT))
        {
            SHORT v = *reinterpret_cast<const SHORT*>(data);
            ss << v;
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_BYTE:
        if (size >= sizeof(BYTE))
        {
            BYTE v = *reinterpret_cast<const BYTE*>(data);
            ss << static_cast<unsigned>(v);
            return ss.str();
        }
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_GUID:
        if (size >= sizeof(GUID))
            return GuidToString(*reinterpret_cast<const GUID*>(data));
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_FILETIME:
        if (size >= sizeof(FILETIME))
            return FileTimeToString(*reinterpret_cast<const FILETIME*>(data));
        return L"(dimensiune invalida)";

    case DEVPROP_TYPE_DEVPROPKEY:
        if (size >= sizeof(DEVPROPKEY))
        {
            const DEVPROPKEY& k = *reinterpret_cast<const DEVPROPKEY*>(data);
            ss << GuidToString(k.fmtid) << L", PID=" << k.pid;
            return ss.str();
        }
        return L"(dimensiune invalida)";

    default:
        return BinaryToHex(data, size);
    }
}

std::wstring PropertyKeyFriendlyName(const DEVPROPKEY& key)
{
    if (EqualPropKey(key, DEVPKEY_Device_DeviceDesc)) return L"DEVPKEY_Device_DeviceDesc";
    if (EqualPropKey(key, DEVPKEY_Device_FriendlyName)) return L"DEVPKEY_Device_FriendlyName";
    if (EqualPropKey(key, DEVPKEY_Device_Manufacturer)) return L"DEVPKEY_Device_Manufacturer";
    if (EqualPropKey(key, DEVPKEY_Device_Class)) return L"DEVPKEY_Device_Class";
    if (EqualPropKey(key, DEVPKEY_Device_ClassGuid)) return L"DEVPKEY_Device_ClassGuid";
    if (EqualPropKey(key, DEVPKEY_Device_HardwareIds)) return L"DEVPKEY_Device_HardwareIds";
    if (EqualPropKey(key, DEVPKEY_Device_CompatibleIds)) return L"DEVPKEY_Device_CompatibleIds";
    if (EqualPropKey(key, DEVPKEY_Device_Service)) return L"DEVPKEY_Device_Service";
    if (EqualPropKey(key, DEVPKEY_Device_DriverVersion)) return L"DEVPKEY_Device_DriverVersion";
    if (EqualPropKey(key, DEVPKEY_Device_LocationInfo)) return L"DEVPKEY_Device_LocationInfo";
    if (EqualPropKey(key, DEVPKEY_Device_LocationPaths)) return L"DEVPKEY_Device_LocationPaths";
    if (EqualPropKey(key, DEVPKEY_Device_EnumeratorName)) return L"DEVPKEY_Device_EnumeratorName";
    if (EqualPropKey(key, DEVPKEY_Device_InstanceId)) return L"DEVPKEY_Device_InstanceId";
    if (EqualPropKey(key, DEVPKEY_Device_ContainerId)) return L"DEVPKEY_Device_ContainerId";
    if (EqualPropKey(key, DEVPKEY_Device_Parent)) return L"DEVPKEY_Device_Parent";
    if (EqualPropKey(key, DEVPKEY_Device_Children)) return L"DEVPKEY_Device_Children";
    if (EqualPropKey(key, DEVPKEY_Device_Siblings)) return L"DEVPKEY_Device_Siblings";

    return L"(cheie nemapata in exemplu)";
}

bool GetStringProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfo, const DEVPROPKEY& key, std::wstring& out)
{
    DEVPROPTYPE type = DEVPROP_TYPE_EMPTY;
    DWORD requiredSize = 0;

    SetupDiGetDevicePropertyW(hDevInfo, &devInfo, &key, &type, nullptr, 0, &requiredSize, 0);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return false;

    std::vector<BYTE> buffer(requiredSize);
    if (!SetupDiGetDevicePropertyW(hDevInfo, &devInfo, &key, &type,
        buffer.data(), requiredSize, nullptr, 0))
        return false;

    if (type != DEVPROP_TYPE_STRING)
        return false;

    out = reinterpret_cast<const wchar_t*>(buffer.data());
    return true;
}

std::wstring GetInstanceId(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfo)
{
    DWORD required = 0;
    SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfo, nullptr, 0, &required);

    if (required == 0)
        return L"(Instance ID indisponibil)";

    std::vector<wchar_t> buffer(required + 1, L'\0');
    if (!SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfo, buffer.data(),
        static_cast<DWORD>(buffer.size()), nullptr))
        return L"(Instance ID indisponibil)";

    return buffer.data();
}

std::wstring GetDisplayName(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfo)
{
    std::wstring text;

    if (GetStringProperty(hDevInfo, devInfo, DEVPKEY_Device_FriendlyName, text))
        return text;

    if (GetStringProperty(hDevInfo, devInfo, DEVPKEY_Device_DeviceDesc, text))
        return text;

    return GetInstanceId(hDevInfo, devInfo);
}

void DumpAllProperties(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfo)
{
    DWORD keyCount = 0;
    SetupDiGetDevicePropertyKeys(hDevInfo, &devInfo, nullptr, 0, &keyCount, 0);

    DWORD err = GetLastError();
    if (err != ERROR_INSUFFICIENT_BUFFER && keyCount != 0)
    {
        std::wcerr << L"Eroare la obtinerea numarului de proprietati. Cod: " << err << L"\n";
        return;
    }

    std::vector<DEVPROPKEY> keys(keyCount);
    if (keyCount > 0)
    {
        if (!SetupDiGetDevicePropertyKeys(hDevInfo, &devInfo,
            keys.data(), keyCount, &keyCount, 0))
        {
            std::wcerr << L"Eroare la citirea cheilor de proprietati. Cod: "
                << GetLastError() << L"\n";
            return;
        }
    }

    std::wcout << L"\n============================================================\n";
    std::wcout << L"Total proprietati gasite: " << keyCount << L"\n";
    std::wcout << L"============================================================\n";

    for (DWORD i = 0; i < keyCount; ++i)
    {
        DEVPROPTYPE type = DEVPROP_TYPE_EMPTY;
        DWORD requiredSize = 0;

        BOOL ok = SetupDiGetDevicePropertyW(hDevInfo, &devInfo, &keys[i],
            &type, nullptr, 0, &requiredSize, 0);

        DWORD firstErr = ok ? ERROR_SUCCESS : GetLastError();

        std::vector<BYTE> buffer(requiredSize ? requiredSize : 1);

        if (!ok && firstErr != ERROR_INSUFFICIENT_BUFFER)
        {
            std::wcout << L"\n[" << i << L"] " << PropertyKeyFriendlyName(keys[i]) << L"\n";
            std::wcout << L"Cheie : " << GuidToString(keys[i].fmtid) << L", PID=" << keys[i].pid << L"\n";
            std::wcout << L"Eroare la citire: " << firstErr << L"\n";
            continue;
        }

        if (!SetupDiGetDevicePropertyW(hDevInfo, &devInfo, &keys[i], &type,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &requiredSize, 0))
        {
            std::wcout << L"\n[" << i << L"] " << PropertyKeyFriendlyName(keys[i]) << L"\n";
            std::wcout << L"Cheie : " << GuidToString(keys[i].fmtid) << L", PID=" << keys[i].pid << L"\n";
            std::wcout << L"Eroare la citire: " << GetLastError() << L"\n";
            continue;
        }

        std::wcout << L"\n[" << i << L"] " << PropertyKeyFriendlyName(keys[i]) << L"\n";
        std::wcout << L"Cheie : " << GuidToString(keys[i].fmtid) << L", PID=" << keys[i].pid << L"\n";
        std::wcout << L"Tip   : " << TypeToString(type) << L"\n";
        std::wcout << L"Valoare: " << ValueToString(type, buffer.data(), requiredSize) << L"\n";
    }
}

int wmain()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    HDEVINFO hDevInfo = SetupDiGetClassDevsW(
        nullptr,
        nullptr,
        nullptr,
        DIGCF_ALLCLASSES | DIGCF_PRESENT
    );

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"Nu s-a putut obtine lista de device-uri. Cod eroare: "
            << GetLastError() << L"\n";
        return 1;
    }

    std::vector<SP_DEVINFO_DATA> devices;
    SP_DEVINFO_DATA devInfo{};
    devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

    std::wcout << L"Lista device-uri prezente in sistem:\n";
    std::wcout << L"------------------------------------------------------------\n";

    for (DWORD index = 0; SetupDiEnumDeviceInfo(hDevInfo, index, &devInfo); ++index)
    {
        devices.push_back(devInfo);
        std::wcout << L"[" << index << L"] " << GetDisplayName(hDevInfo, devices.back()) << L"\n";
    }

    if (GetLastError() != ERROR_NO_MORE_ITEMS)
    {
        std::wcerr << L"Eroare la enumerarea device-urilor. Cod eroare: "
            << GetLastError() << L"\n";
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return 1;
    }

    if (devices.empty())
    {
        std::wcout << L"Nu exista device-uri prezente.\n";
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return 0;
    }

    std::wcout << L"\nAlege indexul device-ului: ";
    size_t selected = 0;
    std::wcin >> selected;

    if (!std::wcin || selected >= devices.size())
    {
        std::wcerr << L"Index invalid.\n";
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return 1;
    }

    std::wcout << L"\nDevice selectat: " << GetDisplayName(hDevInfo, devices[selected]) << L"\n";
    std::wcout << L"Instance ID    : " << GetInstanceId(hDevInfo, devices[selected]) << L"\n";

    DumpAllProperties(hDevInfo, devices[selected]);

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0;
}