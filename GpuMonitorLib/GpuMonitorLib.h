#pragma once

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "exttools.h"

#include <codecvt>
#include <vector>

typedef ET_PROCESS_GPU_STATISTICS GPU_INFO;

class GpuMonitorLib
{
public:

	GpuMonitorLib()
	{
		
	}

	~GpuMonitorLib()
	{

	}

	HRESULT InitializeGpuMonitor(std::wstring ApplicationName = L"GpuMonitorLib")
	{
		HINSTANCE Instance = GetModuleHandle(0);

		if (Instance == NULL)
		{
			return -1;
		}

		if (ApplicationName == L"")
			ApplicationName = L"GpuMonitorLib";

		size_t strLength = ApplicationName.length();
		wchar_t* appName = new wchar_t[strLength + 1];
		wcscpy(appName, ApplicationName.c_str());
		appName[strLength] = L'\0';

		if (!NT_SUCCESS(PhInitializePhLib(appName, Instance)))
		{
			delete[] appName;
			return -2;
		}

		//Init Gpu Monitor
		EtGpuMonitorInitialization();
		//Init Gpu Performance Counter
		EtPerfCounterInitialization();

		//Clean And Update Gpu List
		m_gpuList.clear();
		ULONG cnt = EtGetGpuAdapterCount();
		for (ULONG i = 0; i < cnt; ++i)
		{
			GPU_INFO single_gpu_info;
			EtGetGpuStatistics(i, &single_gpu_info);
			m_gpuList.emplace_back(single_gpu_info);
		}

		delete[] appName;
		return 0;
	}

	void UpdatePerfDatas()
	{
		EtUpdatePerfCounterData();
	}

	int GpuCount()
	{
		return m_gpuList.size();
	}

	int GpuNodeCount(size_t gpuIndex)
	{
		if (gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return -1;
		}

		return m_gpuList[gpuIndex].NodeCount;
	}

	std::wstring GetGpuName(size_t gpuIndex)
	{
		if(gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return L"";
		}

		return CastPhStringW(m_gpuList[gpuIndex].GpuDescription, false);
	}

	std::wstring GetGpuNodeName(size_t gpuIndex, size_t gpuNodeIndex)
	{
		if (gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return L"";
		}
		if (gpuNodeIndex >= m_gpuList[gpuIndex].NodeCount)
		{
			throw L"ERR: Invalid gpuNodeIndex.";
			return L"";
		}

		return CastPhStringW(EtGetGpuAdapterNodeDescription(gpuIndex, gpuNodeIndex), false);
	}

	//0-100%
	float GetSystemGpuUtilization()
	{
		return EtLookupTotalGpuUtilization() * 100;
	}

	//0-100%
	float GetGpuUtilization(size_t gpuIndex)
	{
		if (gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return 0.f;
		}

		return EtLookupTotalGpuAdapterUtilization(m_gpuList[gpuIndex].GpuAdapterLuid) * 100;
	}

	//0-100%
	float GetGpuNodeUtilization(size_t gpuIndex, size_t gpuNodeIndex)
	{
		if (gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return 0.f;
		}
		if (gpuNodeIndex >= m_gpuList[gpuIndex].NodeCount)
		{
			throw L"ERR: Invalid gpuNodeIndex.";
			return 0.f;
		}

		return EtLookupTotalGpuAdapterEngineUtilization(m_gpuList[gpuIndex].GpuAdapterLuid, gpuNodeIndex) * 100;
	}

	//Bytes Dedicated
	uint64_t GetGpuDedicatedMemory(size_t gpuIndex)
	{
		if (gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return 0;
		}

		return EtLookupTotalGpuAdapterDedicated(m_gpuList[gpuIndex].GpuAdapterLuid);
	}

	//Bytes Shared
	uint64_t GetGpuSharedMemory(size_t gpuIndex)
	{
		if (gpuIndex >= m_gpuList.size())
		{
			throw L"ERR: Invalid gpuIndex.";
			return 0;
		}

		return EtLookupTotalGpuAdapterShared(m_gpuList[gpuIndex].GpuAdapterLuid);
	}

	//0-100%
	float GetProcessGpuUtilization(HANDLE hProcess)
	{
		return EtLookupProcessGpuUtilization(hProcess) * 100;
	}
	

	bool GetProcessGpuMemoryUtilization(HANDLE hProcess, uint64_t& SharedUsage, uint64_t& DedicatedUsage, uint64_t& CommitedUsage)
	{
		return EtLookupProcessGpuMemoryCounters(hProcess, &SharedUsage, &DedicatedUsage, &CommitedUsage);
	}

private:

	std::wstring CastPhStringW(PPH_STRING phString, bool bDeRef)
	{
		std::wstring result;
		if (phString)
		{
			result.assign(phString->Buffer, phString->Length / sizeof(wchar_t));

			if (bDeRef)
				PhDereferenceObject(phString);
		}
		return result;
	}

	std::vector<GPU_INFO> m_gpuList;

};

