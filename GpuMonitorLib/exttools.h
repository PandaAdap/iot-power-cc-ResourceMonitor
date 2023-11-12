#ifndef EXTTOOLS_H
#define EXTTOOLS_H

#include <ph.h>
#include <phnet.h>
#include <dltmgr.h>

#include <mapldr.h>

// d3dkmddi requires the WDK (dmex)
#if defined(NTDDI_WIN10_CO) && (NTDDI_VERSION >= NTDDI_WIN10_CO)
#ifdef __has_include
#if __has_include (<dxmini.h>) && \
__has_include (<d3dkmddi.h>) && \
__has_include (<d3dkmthk.h>)
#include <dxmini.h>
#include <d3dkmddi.h>
#include <d3dkmthk.h>
#else
#include "d3dkmt/d3dkmthk.h"
#endif
#else
#include "d3dkmt/d3dkmthk.h"
#endif
#else
#include "d3dkmt/d3dkmthk.h"
#endif

#ifdef __cplusplus
#define EXT_C_FUNC EXTERN_C
#else
#define EXT_C_FUNC 
#endif

// basic type def

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS* PNTSTATUS;
typedef unsigned long ULONG;
typedef BYTE BOOLEAN;
typedef void* PVOID;
typedef void* POINTER_64 PVOID64;
typedef D3DKMT_HANDLE* PD3DKMT_HANDLE;

typedef struct _ET_PROCESS_GPU_STATISTICS
{
    PPH_STRING GpuDescription;
    LUID GpuAdapterLuid;

    ULONG SegmentCount;
    ULONG NodeCount;

    ULONG64 DedicatedCommitted;
    ULONG64 SharedCommitted;

    ULONG64 BytesAllocated;
    ULONG64 BytesReserved;
    ULONG64 WriteCombinedBytesAllocated;
    ULONG64 WriteCombinedBytesReserved;
    ULONG64 CachedBytesAllocated;
    ULONG64 CachedBytesReserved;
    ULONG64 SectionBytesAllocated;
    ULONG64 SectionBytesReserved;

    ULONG64 RunningTime;
    ULONG64 ContextSwitches;
} ET_PROCESS_GPU_STATISTICS, * PET_PROCESS_GPU_STATISTICS;

// gpumon

EXT_C_FUNC int fnGpuMonitorLib();

EXT_C_FUNC VOID EtGpuMonitorInitialization(
    VOID
    );

EXT_C_FUNC ULONG EtGetGpuAdapterCount(
    VOID
    );

EXT_C_FUNC ULONG EtGetGpuAdapterIndexFromNodeIndex(
    _In_ ULONG NodeIndex
    );

EXT_C_FUNC PPH_STRING EtGetGpuAdapterNodeDescription(
    _In_ ULONG Index,
    _In_ ULONG NodeIndex
    );

EXT_C_FUNC PPH_STRING EtGetGpuAdapterDescription(
    _In_ ULONG Index
    );

EXT_C_FUNC VOID EtGetGpuStatistics(
    _In_ ULONG AdapterIndex,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
);

EXT_C_FUNC VOID EtQueryProcessGpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
    );

// counters

EXT_C_FUNC VOID EtPerfCounterInitialization(
    VOID
    );

EXT_C_FUNC NTSTATUS EtUpdatePerfCounterData(
    VOID
    );

EXT_C_FUNC FLOAT EtLookupProcessGpuUtilization(
    _In_ HANDLE ProcessId
    );

_Success_(return)
EXT_C_FUNC BOOLEAN EtLookupProcessGpuMemoryCounters(
    _In_opt_ HANDLE ProcessId,
    _Out_ PULONG64 SharedUsage,
    _Out_ PULONG64 DedicatedUsage,
    _Out_ PULONG64 CommitUsage
    );

EXT_C_FUNC FLOAT EtLookupTotalGpuUtilization(
    VOID
    );

EXT_C_FUNC FLOAT EtLookupTotalGpuEngineUtilization(
    _In_ ULONG EngineId
    );

EXT_C_FUNC FLOAT EtLookupTotalGpuAdapterUtilization(
    _In_ LUID AdapterLuid
    );

EXT_C_FUNC FLOAT EtLookupTotalGpuAdapterEngineUtilization(
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    );

EXT_C_FUNC ULONG64 EtLookupTotalGpuDedicated(
    VOID
    );

EXT_C_FUNC ULONG64 EtLookupTotalGpuAdapterDedicated(
    _In_ LUID AdapterLuid
    );

EXT_C_FUNC ULONG64 EtLookupTotalGpuShared(
    VOID
    );

EXT_C_FUNC ULONG64 EtLookupTotalGpuAdapterShared(
    _In_ LUID AdapterLuid
    );

#endif
