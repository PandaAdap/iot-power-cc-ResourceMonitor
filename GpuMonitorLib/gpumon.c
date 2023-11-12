#include "exttools.h"
#include <cfgmgr32.h>
#include <ntddvdeo.h>
#include <d3d11.h>
#include "gpumon.h"

ULONG EtWindowsVersion = WINDOWS_ANCIENT;
static WCHAR WindowsVersionStringBuffer[40] = { L'0', L'.', L'0', L'.', L'0', UNICODE_NULL };

BOOLEAN EtGpuEnabled = FALSE;
BOOLEAN EtGpuSupported = TRUE;
BOOLEAN EtD3DEnabled = FALSE;
PPH_LIST EtpGpuAdapterList;

ULONG EtGpuTotalNodeCount = 0;
ULONG EtGpuTotalSegmentCount = 0;
ULONG EtGpuNextNodeIndex = 0;

PH_UINT64_DELTA EtClockTotalRunningTimeDelta = { 0, 0 };
LARGE_INTEGER EtClockTotalRunningTimeFrequency = { 0 };

FLOAT EtGpuNodeUsage = 0;

PPH_UINT64_DELTA EtGpuNodesTotalRunningTimeDelta;

ULONG64 EtGpuDedicatedLimit = 0;
ULONG64 EtGpuDedicatedUsage = 0;
ULONG64 EtGpuSharedLimit = 0;
ULONG64 EtGpuSharedUsage = 0;
FLOAT EtGpuPowerUsageLimit = 100.0f;
FLOAT EtGpuPowerUsage = 0.0f;
FLOAT EtGpuTemperatureLimit = 0.0f;
FLOAT EtGpuTemperature = 0.0f;
ULONG64 EtGpuFanRpmLimit = 0;
ULONG64 EtGpuFanRpm = 0;

ULONG GetWindowsVersion(
    VOID
)
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    PH_FORMAT format[5];

    memset(&versionInfo, 0, sizeof(RTL_OSVERSIONINFOEXW));
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion(&versionInfo)))
    {
        return WINDOWS_ANCIENT;
        //WindowsVersionName = L"Windows";
    }

    PhInitFormatU(&format[0], versionInfo.dwMajorVersion);
    PhInitFormatC(&format[1], L'.');
    PhInitFormatU(&format[2], versionInfo.dwMinorVersion);
    PhInitFormatC(&format[3], L'.');
    PhInitFormatU(&format[4], versionInfo.dwBuildNumber);
    PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        WindowsVersionStringBuffer,
        sizeof(WindowsVersionStringBuffer),
        NULL
    );

    memcpy(&PhOsVersion, &versionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    majorVersion = versionInfo.dwMajorVersion;
    minorVersion = versionInfo.dwMinorVersion;
    buildVersion = versionInfo.dwBuildNumber;

    if (majorVersion == 6 && minorVersion < 1 || majorVersion < 6)
    {
        return WINDOWS_ANCIENT;
        //WindowsVersionName = L"Windows";
    }
    // Windows 7, Windows Server 2008 R2
    else if (majorVersion == 6 && minorVersion == 1)
    {
        return WINDOWS_7;
        //WindowsVersionName = L"Windows 7";
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2)
    {
        return WINDOWS_8;
        //WindowsVersionName = L"Windows 8";
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3)
    {
        return WINDOWS_8_1;
        //WindowsVersionName = L"Windows 8.1";
    }
    // Windows 10, Windows Server 2016
    else if (majorVersion == 10 && minorVersion == 0)
    {
        if (buildVersion > 22631)
        {
            return WINDOWS_NEW;
            //WindowsVersionName = L"Windows";
        }
        else if (buildVersion >= 22631)
        {
            return WINDOWS_11_23H2;
            //WindowsVersionName = L"Windows 11 23H2";
        }
        else if (buildVersion >= 22621)
        {
            return WINDOWS_11_22H2;
            //WindowsVersionName = L"Windows 11 22H2";
        }
        else if (buildVersion >= 22000)
        {
            return WINDOWS_11;
            //WindowsVersionName = L"Windows 11";
        }
        else if (buildVersion >= 19045)
        {
            return WINDOWS_10_22H2;
            //WindowsVersionName = L"Windows 10 22H2";
        }
        else if (buildVersion >= 19044)
        {
            return WINDOWS_10_21H2;
            //WindowsVersionName = L"Windows 10 21H2";
        }
        else if (buildVersion >= 19043)
        {
            return WINDOWS_10_21H1;
            //WindowsVersionName = L"Windows 10 21H1";
        }
        else if (buildVersion >= 19042)
        {
            return WINDOWS_10_20H2;
            //WindowsVersionName = L"Windows 10 20H2";
        }
        else if (buildVersion >= 19041)
        {
            return WINDOWS_10_20H1;
            //WindowsVersionName = L"Windows 10 20H1";
        }
        else if (buildVersion >= 18363)
        {
            return WINDOWS_10_19H2;
            //WindowsVersionName = L"Windows 10 19H2";
        }
        else if (buildVersion >= 18362)
        {
            return WINDOWS_10_19H1;
            //WindowsVersionName = L"Windows 10 19H1";
        }
        else if (buildVersion >= 17763)
        {
            return WINDOWS_10_RS5;
            //WindowsVersionName = L"Windows 10 RS5";
        }
        else if (buildVersion >= 17134)
        {
            return WINDOWS_10_RS4;
            //WindowsVersionName = L"Windows 10 RS4";
        }
        else if (buildVersion >= 16299)
        {
            return WINDOWS_10_RS3;
            //WindowsVersionName = L"Windows 10 RS3";
        }
        else if (buildVersion >= 15063)
        {
            return WINDOWS_10_RS2;
            //WindowsVersionName = L"Windows 10 RS2";
        }
        else if (buildVersion >= 14393)
        {
            return WINDOWS_10_RS1;
            //WindowsVersionName = L"Windows 10 RS1";
        }
        else if (buildVersion >= 10586)
        {
            return WINDOWS_10_TH2;
            //WindowsVersionName = L"Windows 10 TH2";
        }
        else if (buildVersion >= 10240)
        {
            return WINDOWS_10;
            //WindowsVersionName = L"Windows 10";
        }
        else
        {
            return WINDOWS_10;
            //WindowsVersionName = L"Windows 10";
        }
    }
    else
    {
        return WINDOWS_NEW;
        //WindowsVersionName = L"Windows";
    }
}

VOID EtGpuMonitorInitialization(
    VOID
    )
{

    EtWindowsVersion = GetWindowsVersion();

    EtD3DEnabled = EtGpuSupported;
    EtpGpuAdapterList = PhCreateList(4);

    if (EtpInitializeD3DStatistics())
        EtGpuEnabled = TRUE;

    if (EtD3DEnabled)
    {
        EtPerfCounterInitialization();
    }

    if (EtGpuEnabled)
    {
        if (!EtD3DEnabled)
            EtGpuNodesTotalRunningTimeDelta = PhAllocateZero(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
    }
}

NTSTATUS EtQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
    )
{
    D3DKMT_QUERYADAPTERINFO queryAdapterInfo;

    memset(&queryAdapterInfo, 0, sizeof(D3DKMT_QUERYADAPTERINFO));
    queryAdapterInfo.hAdapter = AdapterHandle;
    queryAdapterInfo.Type = InformationClass;
    queryAdapterInfo.pPrivateDriverData = Information;
    queryAdapterInfo.PrivateDriverDataSize = InformationLength;

    return D3DKMTQueryAdapterInfo(&queryAdapterInfo);
}

_Success_(return)
BOOLEAN EtOpenAdapterFromDeviceName(
    _Out_ PD3DKMT_HANDLE AdapterHandle,
    _In_ PWSTR DeviceName
    )
{
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;

    memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMDEVICENAME));
    openAdapterFromDeviceName.pDeviceName = DeviceName;

    if (NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
    {
        *AdapterHandle = openAdapterFromDeviceName.hAdapter;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN EtCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_CLOSEADAPTER closeAdapter;

    memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
    closeAdapter.hAdapter = AdapterHandle;

    return NT_SUCCESS(D3DKMTCloseAdapter(&closeAdapter));
}

BOOLEAN EtpIsGpuSoftwareDevice(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_ADAPTERTYPE adapterType;

    memset(&adapterType, 0, sizeof(D3DKMT_ADAPTERTYPE));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERTYPE,
        &adapterType,
        sizeof(D3DKMT_ADAPTERTYPE)
        )))
    {
        if (adapterType.SoftwareDevice) // adapterType.HybridIntegrated
        {
            return TRUE;
        }
    }

    return FALSE;
}

PPH_STRING EtpGetNodeEngineTypeString(
    _In_ D3DKMT_NODEMETADATA NodeMetaData
    )
{
    switch (NodeMetaData.NodeData.EngineType)
    {
    case DXGK_ENGINE_TYPE_OTHER:
        return PhCreateString(NodeMetaData.NodeData.FriendlyName);
    case DXGK_ENGINE_TYPE_3D:
        return PhCreateString(L"3D");
    case DXGK_ENGINE_TYPE_VIDEO_DECODE:
        return PhCreateString(L"Video Decode");
    case DXGK_ENGINE_TYPE_VIDEO_ENCODE:
        return PhCreateString(L"Video Encode");
    case DXGK_ENGINE_TYPE_VIDEO_PROCESSING:
        return PhCreateString(L"Video Processing");
    case DXGK_ENGINE_TYPE_SCENE_ASSEMBLY:
        return PhCreateString(L"Scene Assembly");
    case DXGK_ENGINE_TYPE_COPY:
        return PhCreateString(L"Copy");
    case DXGK_ENGINE_TYPE_OVERLAY:
        return PhCreateString(L"Overlay");
    case DXGK_ENGINE_TYPE_CRYPTO:
        return PhCreateString(L"Crypto");
    }

    return PhFormatString(L"ERROR (%lu)", NodeMetaData.NodeData.EngineType);
}

PVOID EtpQueryDeviceProperty(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY* DeviceProperty
    )
{
    CONFIGRET result;
    PBYTE buffer;
    ULONG bufferSize;
    DEVPROPTYPE propertyType;

    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize);
    propertyType = DEVPROP_TYPE_EMPTY;

    if ((result = CM_Get_DevNode_Property(
        DeviceHandle,
        DeviceProperty,
        &propertyType,
        buffer,
        &bufferSize,
        0
        )) == CR_BUFFER_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        result = CM_Get_DevNode_Property(
            DeviceHandle,
            DeviceProperty,
            &propertyType,
            buffer,
            &bufferSize,
            0
            );
    }

    if (result == CR_SUCCESS)
        return buffer;

    PhFree(buffer);
    return NULL;
}

PPH_STRING EtpQueryDevicePropertyString(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY *DeviceProperty
    )
{
    CONFIGRET result;
    PBYTE buffer;
    ULONG bufferSize;
    DEVPROPTYPE propertyType;

    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize);
    propertyType = DEVPROP_TYPE_EMPTY;

    if ((result = CM_Get_DevNode_Property(
        DeviceHandle,
        DeviceProperty,
        &propertyType,
        buffer,
        &bufferSize,
        0
        )) == CR_BUFFER_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        result = CM_Get_DevNode_Property(
            DeviceHandle,
            DeviceProperty,
            &propertyType,
            buffer,
            &bufferSize,
            0
            );
    }

    if (result != CR_SUCCESS)
    {
        PhFree(buffer);
        return NULL;
    }

    switch (propertyType)
    {
    case DEVPROP_TYPE_STRING:
        {
            PPH_STRING string;

            string = PhCreateStringEx((PWCHAR)buffer, bufferSize);
            PhTrimToNullTerminatorString(string);

            PhFree(buffer);
            return string;
        }
        break;
    case DEVPROP_TYPE_FILETIME:
        {
            PPH_STRING string;
            PFILETIME fileTime;
            LARGE_INTEGER time;
            SYSTEMTIME systemTime;

            fileTime = (PFILETIME)buffer;
            time.HighPart = fileTime->dwHighDateTime;
            time.LowPart = fileTime->dwLowDateTime;

            PhLargeIntegerToLocalSystemTime(&systemTime, &time);

            string = PhFormatDate(&systemTime, NULL);

            //FILETIME newFileTime;
            //SYSTEMTIME systemTime;
            //
            //FileTimeToLocalFileTime((PFILETIME)buffer, &newFileTime);
            //FileTimeToSystemTime(&newFileTime, &systemTime);
            //
            //string = PhFormatDate(&systemTime, NULL);

            PhFree(buffer);
            return string;
        }
        break;
    case DEVPROP_TYPE_UINT32:
        {
            PPH_STRING string;

            string = PhFormatUInt64(*(PULONG)buffer, FALSE);

            PhFree(buffer);
            return string;
        }
        break;
    case DEVPROP_TYPE_UINT64:
        {
            PPH_STRING string;

            string = PhFormatUInt64(*(PULONG64)buffer, FALSE);

            PhFree(buffer);
            return string;
        }
        break;
    }

    return NULL;
}

PPH_STRING EtpQueryDeviceRegistryProperty(
    _In_ DEVINST DeviceHandle,
    _In_ ULONG DeviceProperty
    )
{
    CONFIGRET result;
    PPH_STRING string;
    ULONG bufferSize;
    DEVPROPTYPE devicePropertyType;

    bufferSize = 0x80;
    string = PhCreateStringEx(NULL, bufferSize);

    if ((result = CM_Get_DevNode_Registry_Property(
        DeviceHandle,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)string->Buffer,
        &bufferSize,
        0
        )) == CR_BUFFER_SMALL)
    {
        PhDereferenceObject(string);
        string = PhCreateStringEx(NULL, bufferSize);

        result = CM_Get_DevNode_Registry_Property(
            DeviceHandle,
            DeviceProperty,
            &devicePropertyType,
            (PBYTE)string->Buffer,
            &bufferSize,
            0
            );
    }

    if (result != CR_SUCCESS)
    {
        PhDereferenceObject(string);
        return NULL;
    }

    PhTrimToNullTerminatorString(string);

    return string;
}

ULONG64 EtpQueryGpuInstalledMemory(
    _In_ DEVINST DeviceHandle
    )
{
    ULONG64 installedMemory = ULLONG_MAX;
    HKEY keyHandle;

    if (CM_Open_DevInst_Key(
        DeviceHandle,
        KEY_READ,
        0,
        RegDisposition_OpenExisting,
        &keyHandle,
        CM_REGISTRY_SOFTWARE
        ) == CR_SUCCESS)
    {
        installedMemory = PhQueryRegistryUlong64Z(keyHandle, L"HardwareInformation.qwMemorySize");

        if (installedMemory == ULLONG_MAX)
            installedMemory = PhQueryRegistryUlongZ(keyHandle, L"HardwareInformation.MemorySize");

        if (installedMemory == ULONG_MAX) // HACK
            installedMemory = ULLONG_MAX;

        // Intel GPU devices incorrectly create the key with type REG_BINARY.
        if (installedMemory == ULLONG_MAX)
        {
            static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"HardwareInformation.MemorySize");
            PKEY_VALUE_PARTIAL_INFORMATION buffer;

            if (NT_SUCCESS(PhQueryValueKey(keyHandle, &valueName, KeyValuePartialInformation, &buffer)))
            {
                if (buffer->Type == REG_BINARY)
                {
                    if (buffer->DataLength == sizeof(ULONG))
                        installedMemory = *(PULONG)buffer->Data;
                }

                PhFree(buffer);
            }
        }

        NtClose(keyHandle);
    }

    return installedMemory;
}

_Success_(return)
BOOLEAN EtQueryDeviceProperties(
    _In_ PPH_STRING DeviceInterface,
    _Out_opt_ PPH_STRING *Description,
    _Out_opt_ PPH_STRING *DriverDate,
    _Out_opt_ PPH_STRING *DriverVersion,
    _Out_opt_ PPH_STRING *LocationInfo,
    _Out_opt_ ULONG64 *InstalledMemory
    )
{
    DEVPROPTYPE devicePropertyType;
    DEVINST deviceInstanceHandle;
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

    if (CM_Get_Device_Interface_Property(
        PhGetString(DeviceInterface),
        &DEVPKEY_Device_InstanceId,
        &devicePropertyType,
        (PBYTE)deviceInstanceId,
        &deviceInstanceIdLength,
        0
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        deviceInstanceId,
        CM_LOCATE_DEVNODE_NORMAL
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (Description)
        *Description = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DeviceDesc);
    if (DriverDate)
        *DriverDate = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DriverDate);
    if (DriverVersion)
        *DriverVersion = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_DriverVersion);
    if (LocationInfo)
        *LocationInfo = EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_LocationInfo);
    if (InstalledMemory)
        *InstalledMemory = EtpQueryGpuInstalledMemory(deviceInstanceHandle);
    // EtpQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_Device_Manufacturer);

    return TRUE;
}

PPH_STRING EtQueryDeviceDescription(
    _In_ PPH_STRING DeviceInterface
    )
{
    PPH_STRING description;

    if (EtQueryDeviceProperties(DeviceInterface, &description, NULL, NULL, NULL, NULL))
    {
        return description;
    }

    return NULL;
}

D3D_FEATURE_LEVEL EtQueryAdapterFeatureLevel(
    _In_ LUID AdapterLuid
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice_I = NULL;
    static HRESULT (WINAPI *CreateDXGIFactory1_I)(_In_ REFIID riid, _Out_ PVOID *ppFactory) = NULL;
    D3D_FEATURE_LEVEL d3dFeatureLevelResult = 0;
    IDXGIFactory1 *dxgiFactory;
    IDXGIAdapter* dxgiAdapter;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID dxgi;
        PVOID d3d11;

        if (dxgi = PhLoadLibrary(L"dxgi.dll"))
        {
            CreateDXGIFactory1_I = PhGetProcedureAddress(dxgi, "CreateDXGIFactory1", 0);
        }

        if (d3d11 = PhLoadLibrary(L"d3d11.dll"))
        {
            D3D11CreateDevice_I = PhGetProcedureAddress(d3d11, "D3D11CreateDevice", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!CreateDXGIFactory1_I || !SUCCEEDED(CreateDXGIFactory1_I(&IID_IDXGIFactory1, &dxgiFactory)))
        return 0;

    for (UINT i = 0; i < 25; i++)
    {
        DXGI_ADAPTER_DESC dxgiAdapterDescription;

        if (!SUCCEEDED(IDXGIFactory1_EnumAdapters(dxgiFactory, i, &dxgiAdapter)))
            break;

        if (SUCCEEDED(IDXGIAdapter_GetDesc(dxgiAdapter, &dxgiAdapterDescription)))
        {
            if (RtlIsEqualLuid(&dxgiAdapterDescription.AdapterLuid, &AdapterLuid))
            {
                D3D_FEATURE_LEVEL d3dFeatureLevel[] =
                {
                    D3D_FEATURE_LEVEL_12_1,
                    D3D_FEATURE_LEVEL_12_0,
                    D3D_FEATURE_LEVEL_11_1,
                    D3D_FEATURE_LEVEL_11_0,
                    D3D_FEATURE_LEVEL_10_1,
                    D3D_FEATURE_LEVEL_10_0,
                    D3D_FEATURE_LEVEL_9_3,
                    D3D_FEATURE_LEVEL_9_2,
                    D3D_FEATURE_LEVEL_9_1
                };
                D3D_FEATURE_LEVEL d3ddeviceFeatureLevel;
                ID3D11Device* d3d11device;

                if (D3D11CreateDevice_I && SUCCEEDED(D3D11CreateDevice_I(
                    dxgiAdapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    NULL,
                    0,
                    d3dFeatureLevel,
                    RTL_NUMBER_OF(d3dFeatureLevel),
                    D3D11_SDK_VERSION,
                    &d3d11device,
                    &d3ddeviceFeatureLevel,
                    NULL
                    )))
                {
                    d3dFeatureLevelResult = d3ddeviceFeatureLevel;
                    ID3D11Device_Release(d3d11device);
                    IDXGIAdapter_Release(dxgiAdapter);
                    break;
                }
            }
        }

        IDXGIAdapter_Release(dxgiAdapter);
    }

    IDXGIFactory1_Release(dxgiFactory);
    return d3dFeatureLevelResult;
}

PETP_GPU_ADAPTER EtpAddDisplayAdapter(
    _In_ PPH_STRING DeviceInterface,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ LUID AdapterLuid,
    _In_ ULONG NumberOfSegments,
    _In_ ULONG NumberOfNodes
    )
{
    PETP_GPU_ADAPTER adapter;

    adapter = EtpAllocateGpuAdapter(NumberOfSegments);
    adapter->DeviceInterface = PhReferenceObject(DeviceInterface);
    adapter->AdapterLuid = AdapterLuid;
    adapter->NodeCount = NumberOfNodes;
    adapter->SegmentCount = NumberOfSegments;
    RtlInitializeBitMap(&adapter->ApertureBitMap, adapter->ApertureBitMapBuffer, NumberOfSegments);

    {
        PPH_STRING description;

        if (EtQueryDeviceProperties(DeviceInterface, &description, NULL, NULL, NULL, NULL))
        {
            adapter->Description = description;
        }
    }

    if (EtGpuSupported)
    {
        adapter->NodeNameList = PhCreateList(adapter->NodeCount);

        for (ULONG i = 0; i < adapter->NodeCount; i++)
        {
            D3DKMT_NODEMETADATA metaDataInfo;

            memset(&metaDataInfo, 0, sizeof(D3DKMT_NODEMETADATA));
            metaDataInfo.NodeOrdinalAndAdapterIndex = MAKEWORD(i, 0);

            if (NT_SUCCESS(EtQueryAdapterInformation(
                AdapterHandle,
                KMTQAITYPE_NODEMETADATA,
                &metaDataInfo,
                sizeof(D3DKMT_NODEMETADATA)
                )))
            {
                PhAddItemList(adapter->NodeNameList, EtpGetNodeEngineTypeString(metaDataInfo));
            }
            else
            {
                PhAddItemList(adapter->NodeNameList, PhReferenceEmptyString());
            }
        }
    }

    PhAddItemList(EtpGpuAdapterList, adapter);

    return adapter;
}

BOOLEAN EtpInitializeD3DStatistics(
    VOID
    )
{
    PPH_LIST deviceAdapterList;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    D3DKMT_ADAPTER_PERFDATACAPS perfCaps;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        ) != CR_SUCCESS)
    {
        PhFree(deviceInterfaceList);
        return FALSE;
    }

    deviceAdapterList = PhCreateList(10);
    deviceInterface = deviceInterfaceList;

    while (TRUE)
    {
        PH_STRINGREF string;

        PhInitializeStringRefLongHint(&string, deviceInterface);

        if (string.Length == 0)
            break;

        PhAddItemList(deviceAdapterList, PhCreateString2(&string));

        deviceInterface = PTR_ADD_OFFSET(deviceInterface, string.Length + sizeof(UNICODE_NULL));
    }

    for (ULONG i = 0; i < deviceAdapterList->Count; i++)
    {
        memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMDEVICENAME));
        openAdapterFromDeviceName.pDeviceName = PhGetString(deviceAdapterList->Items[i]);

        if (!NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
            continue;

        if (EtGpuSupported && deviceAdapterList->Count > 1)
        {
            if (EtpIsGpuSoftwareDevice(openAdapterFromDeviceName.hAdapter))
            {
                EtCloseAdapterHandle(openAdapterFromDeviceName.hAdapter);
                continue;
            }
        }

        if (EtGpuSupported)
        {
            D3DKMT_SEGMENTSIZEINFO segmentInfo;

            memset(&segmentInfo, 0, sizeof(D3DKMT_SEGMENTSIZEINFO));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                openAdapterFromDeviceName.hAdapter,
                KMTQAITYPE_GETSEGMENTSIZE,
                &segmentInfo,
                sizeof(D3DKMT_SEGMENTSIZEINFO)
                )))
            {
                EtGpuDedicatedLimit += segmentInfo.DedicatedVideoMemorySize;
                EtGpuSharedLimit += segmentInfo.SharedSystemMemorySize;
            }

            memset(&perfCaps, 0, sizeof(D3DKMT_ADAPTER_PERFDATACAPS));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                openAdapterFromDeviceName.hAdapter,
                KMTQAITYPE_ADAPTERPERFDATA_CAPS,
                &perfCaps,
                sizeof(D3DKMT_ADAPTER_PERFDATACAPS)
                )))
            {
                //
                // This will be averaged below.
                //
                EtGpuTemperatureLimit += max(perfCaps.TemperatureWarning, perfCaps.TemperatureMax);
                EtGpuFanRpmLimit += perfCaps.MaxFanRPM;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
        queryStatistics.AdapterLuid = openAdapterFromDeviceName.AdapterLuid;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            PETP_GPU_ADAPTER gpuAdapter;

            gpuAdapter = EtpAddDisplayAdapter(
                deviceAdapterList->Items[i],
                openAdapterFromDeviceName.hAdapter,
                openAdapterFromDeviceName.AdapterLuid,
                queryStatistics.QueryResult.AdapterInformation.NbSegments,
                queryStatistics.QueryResult.AdapterInformation.NodeCount
                );

            gpuAdapter->FirstNodeIndex = EtGpuNextNodeIndex;
            EtGpuTotalNodeCount += gpuAdapter->NodeCount;
            EtGpuTotalSegmentCount += gpuAdapter->SegmentCount;
            EtGpuNextNodeIndex += gpuAdapter->NodeCount;

            for (ULONG ii = 0; ii < gpuAdapter->SegmentCount; ii++)
            {
                memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
                queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
                queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
                queryStatistics.QuerySegment.SegmentId = ii;

                if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
                {
                    ULONG64 commitLimit;
                    ULONG aperture;

                    if (EtWindowsVersion >= WINDOWS_8)
                    {
                        commitLimit = queryStatistics.QueryResult.SegmentInformation.CommitLimit;
                        aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
                    }
                    else
                    {
                        PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 segmentInfo;

                        segmentInfo = (PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1)&queryStatistics.QueryResult;
                        commitLimit = segmentInfo->CommitLimit;
                        aperture = segmentInfo->Aperture;
                    }

                    if (!EtGpuSupported || !EtD3DEnabled)
                    {
                        if (aperture)
                            EtGpuSharedLimit += commitLimit;
                        else
                            EtGpuDedicatedLimit += commitLimit;
                    }

                    if (aperture)
                        RtlSetBits(&gpuAdapter->ApertureBitMap, ii, 1);
                }
            }
        }

        EtCloseAdapterHandle(openAdapterFromDeviceName.hAdapter);
    }

    if (EtGpuSupported && deviceAdapterList->Count > 0)
    {
        //
        // Use the average as the limit since we show one graph for all.
        //
        EtGpuTemperatureLimit /= deviceAdapterList->Count;
        EtGpuFanRpmLimit /= deviceAdapterList->Count;

        // Set limit at 100C (dmex)
        if (EtGpuTemperatureLimit == 0)
            EtGpuTemperatureLimit = 100;
    }

    PhDereferenceObjects(deviceAdapterList->Items, deviceAdapterList->Count);
    PhDereferenceObject(deviceAdapterList);
    PhFree(deviceInterfaceList);

    if (EtGpuTotalNodeCount == 0)
        return FALSE;

    return TRUE;
}

PETP_GPU_ADAPTER EtpAllocateGpuAdapter(
    _In_ ULONG NumberOfSegments
    )
{
    PETP_GPU_ADAPTER adapter;
    SIZE_T sizeNeeded;

    sizeNeeded = FIELD_OFFSET(ETP_GPU_ADAPTER, ApertureBitMapBuffer);
    sizeNeeded += BYTES_NEEDED_FOR_BITS(NumberOfSegments);

    adapter = PhAllocate(sizeNeeded);
    memset(adapter, 0, sizeNeeded);

    return adapter;
}

/*
VOID EtpUpdateProcessSegmentInformation(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;
    ULONG64 commitUsage;

    if (!Block->ProcessItem->QueryHandle)
        return;

    dedicatedUsage = 0;
    sharedUsage = 0;
    commitUsage = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = Block->ProcessItem->QueryHandle;
            queryStatistics.QueryProcessSegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesCommitted;

                if (EtWindowsVersion >= WINDOWS_8)
                    bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                else
                    bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;

                if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                    sharedUsage += bytesCommitted;
                else
                    dedicatedUsage += bytesCommitted;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS;
        queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
        queryStatistics.hProcess = Block->ProcessItem->QueryHandle;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            commitUsage += queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesAllocated;
        }
    }

    Block->GpuDedicatedUsage = dedicatedUsage;
    Block->GpuSharedUsage = sharedUsage;
    Block->GpuCommitUsage = commitUsage;
}
*/


VOID EtpUpdateSystemSegmentInformation(
    VOID
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 dedicatedUsage;
    ULONG64 sharedUsage;

    dedicatedUsage = 0;
    sharedUsage = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.QuerySegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesResident;
                ULONG aperture;

                if (EtWindowsVersion >= WINDOWS_8)
                {
                    bytesResident = queryStatistics.QueryResult.SegmentInformation.BytesResident;
                    aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
                }
                else
                {
                    PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 segmentInfo;

                    segmentInfo = (PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1)&queryStatistics.QueryResult;
                    bytesResident = segmentInfo->BytesResident;
                    aperture = segmentInfo->Aperture;
                }

                if (aperture) // RtlCheckBit(&gpuAdapter->ApertureBitMap, j)
                    sharedUsage += bytesResident;
                else
                    dedicatedUsage += bytesResident;
            }
        }
    }

    EtGpuDedicatedUsage = dedicatedUsage;
    EtGpuSharedUsage = sharedUsage;
}

/*
VOID EtpUpdateProcessNodeInformation(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    ULONG64 totalRunningTime;

    if (!Block->ProcessItem->QueryHandle)
        return;

    totalRunningTime = 0;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = Block->ProcessItem->QueryHandle;
            queryStatistics.QueryProcessNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                //ULONG64 runningTime;
                //runningTime = queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                //PhUpdateDelta(&Block->GpuTotalRunningTimeDelta[j], runningTime);

                totalRunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                //totalContextSwitches += queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }
    }

    PhUpdateDelta(&Block->GpuRunningTimeDelta, totalRunningTime);
}
*/

VOID EtpUpdateSystemNodeInformation(
    VOID
    )
{
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;
    LARGE_INTEGER performanceCounter;

    for (ULONG i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        for (ULONG j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.QueryNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 runningTime;
                //ULONG64 systemRunningTime;

                runningTime = queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
                //systemRunningTime = queryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;

                PhUpdateDelta(&EtGpuNodesTotalRunningTimeDelta[gpuAdapter->FirstNodeIndex + j], runningTime);
            }
        }
    }

    PhQueryPerformanceCounter(&performanceCounter);
    PhUpdateDelta(&EtClockTotalRunningTimeDelta, performanceCounter.QuadPart);
}

/*
VOID NTAPI EtGpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider
    DOUBLE elapsedTime = 0; // total GPU node elapsed time in micro-seconds
    FLOAT tempGpuUsage = 0;
    ULONG i;
    PLIST_ENTRY listEntry;
    FLOAT maxNodeValue = 0;
    PET_PROCESS_BLOCK maxNodeBlock = NULL;

    if (EtD3DEnabled)
    {
        FLOAT gpuTotal;
        ULONG64 dedicatedTotal;
        ULONG64 sharedTotal;

        EtUpdatePerfCounterData();

        gpuTotal = EtLookupTotalGpuUtilization();
        dedicatedTotal = EtLookupTotalGpuDedicated();
        sharedTotal = EtLookupTotalGpuShared();

        if (gpuTotal > 1)
            gpuTotal = 1;

        if (gpuTotal > tempGpuUsage)
            tempGpuUsage = gpuTotal;

        EtGpuNodeUsage = tempGpuUsage;
        EtGpuDedicatedUsage = dedicatedTotal;
        EtGpuSharedUsage = sharedTotal;
    }
    else
    {
        EtpUpdateSystemSegmentInformation();
        EtpUpdateSystemNodeInformation();

        elapsedTime = (DOUBLE)EtClockTotalRunningTimeDelta.Delta * 10000000 / EtClockTotalRunningTimeFrequency.QuadPart;

        if (elapsedTime != 0)
        {
            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                FLOAT usage = (FLOAT)(EtGpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

                if (usage > 1)
                    usage = 1;

                if (usage > tempGpuUsage)
                    tempGpuUsage = usage;
            }
        }

        EtGpuNodeUsage = tempGpuUsage;
    }

    if (EtGpuSupported && EtpGpuAdapterList->Count)
    {
        FLOAT powerUsage;
        FLOAT temperature;
        ULONG64 fanRpm;

        powerUsage = 0.0f;
        temperature = 0.0f;
        fanRpm = 0;

        for (i = 0; i < EtpGpuAdapterList->Count; i++)
        {
            PETP_GPU_ADAPTER gpuAdapter;
            D3DKMT_HANDLE adapterHandle;
            D3DKMT_ADAPTER_PERFDATA adapterPerfData;

            gpuAdapter = EtpGpuAdapterList->Items[i];

            //
            // jxy-s: we open this frequently, consider opening this once in the list
            //
            if (!NT_SUCCESS(EtOpenAdapterFromDeviceName(&adapterHandle, PhGetString(gpuAdapter->DeviceInterface))))
                continue;

            memset(&adapterPerfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

            if (NT_SUCCESS(EtQueryAdapterInformation(
                adapterHandle,
                KMTQAITYPE_ADAPTERPERFDATA,
                &adapterPerfData,
                sizeof(D3DKMT_ADAPTER_PERFDATA)
                )))
            {
                powerUsage += (((FLOAT)adapterPerfData.Power / 1000) * 100);
                temperature += (((FLOAT)adapterPerfData.Temperature / 1000) * 100);
                fanRpm += adapterPerfData.FanRPM;
            }

            EtCloseAdapterHandle(adapterHandle);
        }

        EtGpuPowerUsage = powerUsage / EtpGpuAdapterList->Count;
        EtGpuTemperature = temperature / EtpGpuAdapterList->Count;
        EtGpuFanRpm = fanRpm / EtpGpuAdapterList->Count;

        //
        // Update the limits if we see higher values
        //

        if (EtGpuPowerUsage > EtGpuPowerUsageLimit)
        {
            //
            // Possibly over-clocked power limit
            //
            EtGpuPowerUsageLimit = EtGpuPowerUsage;
        }

        if (EtGpuTemperature > EtGpuTemperatureLimit)
        {
            //
            // Damn that card is hawt
            //
            EtGpuTemperatureLimit = EtGpuTemperature;
        }

        if (EtGpuFanRpm > EtGpuFanRpmLimit)
        {
            //
            // Fan go brrrrrr
            //
            EtGpuFanRpmLimit = EtGpuFanRpm;
        }
    }

    // Update per-process statistics.
    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

        if (block->ProcessItem->State & PH_PROCESS_ITEM_REMOVED)
        {
            listEntry = listEntry->Flink;
            continue;
        }

        if (EtD3DEnabled)
        {
            ULONG64 sharedUsage;
            ULONG64 dedicatedUsage;
            ULONG64 commitUsage;

            block->GpuNodeUtilization = EtLookupProcessGpuUtilization(block->ProcessItem->ProcessId);

            if (EtLookupProcessGpuMemoryCounters(
                block->ProcessItem->ProcessId,
                &sharedUsage,
                &dedicatedUsage,
                &commitUsage
                ))
            {
                block->GpuSharedUsage = sharedUsage;
                block->GpuDedicatedUsage = dedicatedUsage;
                block->GpuCommitUsage = commitUsage;
            }
            else
            {
                block->GpuSharedUsage = 0;
                block->GpuDedicatedUsage = 0;
                block->GpuCommitUsage = 0;
            }

            if (runCount != 0)
            {
                block->CurrentGpuUsage = block->GpuNodeUtilization;
                block->CurrentMemUsage = (ULONG)(block->GpuDedicatedUsage / PAGE_SIZE);
                block->CurrentMemSharedUsage = (ULONG)(block->GpuSharedUsage / PAGE_SIZE);
                block->CurrentCommitUsage = (ULONG)(block->GpuCommitUsage / PAGE_SIZE);

                PhAddItemCircularBuffer_FLOAT(&block->GpuHistory, block->CurrentGpuUsage);
                PhAddItemCircularBuffer_ULONG(&block->MemoryHistory, block->CurrentMemUsage);
                PhAddItemCircularBuffer_ULONG(&block->MemorySharedHistory, block->CurrentMemSharedUsage);
                PhAddItemCircularBuffer_ULONG(&block->GpuCommittedHistory, block->CurrentCommitUsage);
            }
        }
        else
        {
            EtpUpdateProcessSegmentInformation(block);
            EtpUpdateProcessNodeInformation(block);

            if (elapsedTime != 0)
            {
                block->GpuNodeUtilization = (FLOAT)(block->GpuRunningTimeDelta.Delta / elapsedTime);

                // HACK
                if (block->GpuNodeUtilization > EtGpuNodeUsage)
                    block->GpuNodeUtilization = EtGpuNodeUsage;

                //for (i = 0; i < EtGpuTotalNodeCount; i++)
                //{
                //    FLOAT usage = (FLOAT)(block->GpuTotalRunningTimeDelta[i].Delta / elapsedTime);
                //
                //    if (usage > block->GpuNodeUtilization)
                //    {
                //        block->GpuNodeUtilization = usage;
                //    }
                //}

                if (block->GpuNodeUtilization > 1)
                    block->GpuNodeUtilization = 1;

                if (runCount != 0)
                {
                    block->CurrentGpuUsage = block->GpuNodeUtilization;
                    block->CurrentMemUsage = (ULONG)(block->GpuDedicatedUsage / PAGE_SIZE);
                    block->CurrentMemSharedUsage = (ULONG)(block->GpuSharedUsage / PAGE_SIZE);
                    block->CurrentCommitUsage = (ULONG)(block->GpuCommitUsage / PAGE_SIZE);

                    PhAddItemCircularBuffer_FLOAT(&block->GpuHistory, block->CurrentGpuUsage);
                    PhAddItemCircularBuffer_ULONG(&block->MemoryHistory, block->CurrentMemUsage);
                    PhAddItemCircularBuffer_ULONG(&block->MemorySharedHistory, block->CurrentMemSharedUsage);
                    PhAddItemCircularBuffer_ULONG(&block->GpuCommittedHistory, block->CurrentCommitUsage);
                }
            }
        }

        if (maxNodeValue < block->GpuNodeUtilization)
        {
            maxNodeValue = block->GpuNodeUtilization;
            maxNodeBlock = block;
        }

        listEntry = listEntry->Flink;
    }

    // Update history buffers.

    if (runCount != 0)
    {
        PhAddItemCircularBuffer_FLOAT(&EtGpuNodeHistory, EtGpuNodeUsage);
        PhAddItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, EtGpuDedicatedUsage);
        PhAddItemCircularBuffer_ULONG64(&EtGpuSharedHistory, EtGpuSharedUsage);
        if (EtGpuSupported)
        {
            PhAddItemCircularBuffer_FLOAT(&EtGpuPowerUsageHistory, EtGpuPowerUsage);
            PhAddItemCircularBuffer_FLOAT(&EtGpuTemperatureHistory, EtGpuTemperature);
            PhAddItemCircularBuffer_ULONG64(&EtGpuFanRpmHistory, EtGpuFanRpm);
        }

        if (EtD3DEnabled)
        {
            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                FLOAT usage;

                usage = EtLookupTotalGpuEngineUtilization(i);

                if (usage > 1)
                    usage = 1;

                PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], usage);
            }
        }
        else
        {
            if (elapsedTime != 0)
            {
                for (i = 0; i < EtGpuTotalNodeCount; i++)
                {
                    FLOAT usage;

                    usage = (FLOAT)(EtGpuNodesTotalRunningTimeDelta[i].Delta / elapsedTime);

                    if (usage > 1)
                        usage = 1;

                    PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], usage);
                }
            }
            else
            {
                for (i = 0; i < EtGpuTotalNodeCount; i++)
                    PhAddItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], 0);
            }
        }

        if (maxNodeBlock)
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, HandleToUlong(maxNodeBlock->ProcessItem->ProcessId));
            PhAddItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, maxNodeBlock->GpuNodeUtilization);
            PhReferenceProcessRecordForStatistics(maxNodeBlock->ProcessItem->Record);
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, 0);
            PhAddItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, 0);
        }
    }

    runCount++;
}
*/

ULONG EtGetGpuAdapterCount(
    VOID
    )
{
    return EtpGpuAdapterList->Count;
}

ULONG EtGetGpuAdapterIndexFromNodeIndex(
    _In_ ULONG NodeIndex
    )
{
    ULONG i;
    PETP_GPU_ADAPTER gpuAdapter;

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        if (NodeIndex >= gpuAdapter->FirstNodeIndex && NodeIndex < gpuAdapter->FirstNodeIndex + gpuAdapter->NodeCount)
            return i;
    }

    return ULONG_MAX;
}

PPH_STRING EtGetGpuAdapterNodeDescription(
    _In_ ULONG Index,
    _In_ ULONG NodeIndex
    )
{
    PETP_GPU_ADAPTER gpuAdapter;

    if (Index >= EtpGpuAdapterList->Count)
        return NULL;

    gpuAdapter = EtpGpuAdapterList->Items[Index];

    if (!gpuAdapter->NodeNameList)
        return NULL;

    return gpuAdapter->NodeNameList->Items[NodeIndex - gpuAdapter->FirstNodeIndex];
}

PPH_STRING EtGetGpuAdapterDescription(
    _In_ ULONG Index
    )
{
    PPH_STRING description;

    if (Index >= EtpGpuAdapterList->Count)
        return NULL;

    description = ((PETP_GPU_ADAPTER)EtpGpuAdapterList->Items[Index])->Description;

    if (description)
    {
        PhReferenceObject(description);
        return description;
    }
    else
    {
        return NULL;
    }
}

VOID EtGetGpuStatistics(
    _In_ ULONG AdapterIndex,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
)
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(Statistics, 0, sizeof(ET_PROCESS_GPU_STATISTICS));

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        if (AdapterIndex != i)
            continue;

        gpuAdapter = EtpGpuAdapterList->Items[i];

        Statistics->GpuDescription = gpuAdapter->Description;
        Statistics->GpuAdapterLuid = gpuAdapter->AdapterLuid;
        Statistics->SegmentCount = gpuAdapter->SegmentCount;
        Statistics->NodeCount = gpuAdapter->NodeCount;

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {

            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            //queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessSegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesCommitted;

                if (EtWindowsVersion >= WINDOWS_8)
                    bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                else
                    bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;

                if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                    Statistics->SharedCommitted = bytesCommitted;
                else
                    Statistics->DedicatedCommitted = bytesCommitted;
            }
        }

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            //queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                Statistics->RunningTime = queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                Statistics->ContextSwitches = queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS;
        queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
        //queryStatistics.hProcess = ProcessHandle;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            Statistics->BytesAllocated = queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesAllocated;
            Statistics->BytesReserved = queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesReserved;
            Statistics->WriteCombinedBytesAllocated = queryStatistics.QueryResult.ProcessInformation.SystemMemory.WriteCombinedBytesAllocated;
            Statistics->WriteCombinedBytesReserved = queryStatistics.QueryResult.ProcessInformation.SystemMemory.WriteCombinedBytesReserved;
            Statistics->CachedBytesAllocated = queryStatistics.QueryResult.ProcessInformation.SystemMemory.CachedBytesAllocated;
            Statistics->CachedBytesReserved = queryStatistics.QueryResult.ProcessInformation.SystemMemory.CachedBytesReserved;
            Statistics->SectionBytesAllocated = queryStatistics.QueryResult.ProcessInformation.SystemMemory.SectionBytesAllocated;
            Statistics->SectionBytesReserved = queryStatistics.QueryResult.ProcessInformation.SystemMemory.SectionBytesReserved;
        }
    }
}


VOID EtQueryProcessGpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
    )
{
    ULONG i;
    ULONG j;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(Statistics, 0, sizeof(ET_PROCESS_GPU_STATISTICS));

    for (i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        Statistics->SegmentCount += gpuAdapter->SegmentCount;
        Statistics->NodeCount += gpuAdapter->NodeCount;

        for (j = 0; j < gpuAdapter->SegmentCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessSegment.SegmentId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                ULONG64 bytesCommitted;

                if (EtWindowsVersion >= WINDOWS_8)
                    bytesCommitted = queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
                else
                    bytesCommitted = (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;

                if (RtlCheckBit(&gpuAdapter->ApertureBitMap, j))
                    Statistics->SharedCommitted += bytesCommitted;
                else
                    Statistics->DedicatedCommitted += bytesCommitted;
            }
        }

        for (j = 0; j < gpuAdapter->NodeCount; j++)
        {
            memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
            queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
            queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
            queryStatistics.hProcess = ProcessHandle;
            queryStatistics.QueryProcessNode.NodeId = j;

            if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
            {
                Statistics->RunningTime += queryStatistics.QueryResult.ProcessNodeInformation.RunningTime.QuadPart;
                Statistics->ContextSwitches += queryStatistics.QueryResult.ProcessNodeInformation.ContextSwitch;
            }
        }

        memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
        queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS;
        queryStatistics.AdapterLuid = gpuAdapter->AdapterLuid;
        queryStatistics.hProcess = ProcessHandle;

        if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        {
            Statistics->BytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesAllocated;
            Statistics->BytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.BytesReserved;
            Statistics->WriteCombinedBytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.WriteCombinedBytesAllocated;
            Statistics->WriteCombinedBytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.WriteCombinedBytesReserved;
            Statistics->CachedBytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.CachedBytesAllocated;
            Statistics->CachedBytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.CachedBytesReserved;
            Statistics->SectionBytesAllocated += queryStatistics.QueryResult.ProcessInformation.SystemMemory.SectionBytesAllocated;
            Statistics->SectionBytesReserved += queryStatistics.QueryResult.ProcessInformation.SystemMemory.SectionBytesReserved;
        }
    }
}

D3DKMT_CLIENTHINT EtQueryProcessGpuClientHint(
    _In_ LUID AdapterLuid,
    _In_ HANDLE ProcessHandle
    )
{
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER;
    queryStatistics.AdapterLuid = AdapterLuid;
    queryStatistics.hProcess = ProcessHandle;

    if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
    {
        return queryStatistics.QueryResult.ProcessAdapterInformation.ClientHint;
    }

    return D3DKMT_CLIENTHINT_UNKNOWN;
}
