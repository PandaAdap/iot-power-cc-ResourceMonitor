
// iot-power-cc-ResourceMonitorDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "iot-power-cc-ResourceMonitor.h"
#include "iot-power-cc-ResourceMonitorDlg.h"
#include "afxdialogex.h"

#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CiotpowerccResourceMonitorDlg 对话框



CiotpowerccResourceMonitorDlg::CiotpowerccResourceMonitorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IOTPOWERCCRESOURCEMONITOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CiotpowerccResourceMonitorDlg::~CiotpowerccResourceMonitorDlg()
{

	if (isConnected)
	{
		OnBnClickedApply();
	}

	Shell_NotifyIcon(NIM_DELETE, &m_nid);
}

void CiotpowerccResourceMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_Combo_iotDevices, m_combo_iotdevices);
	DDX_Control(pDX, IDC_Combo_GPUs, m_combo_gpus);
	DDX_Control(pDX, IDC_Combo_RefreshInterval, m_combo_refreshinterval);
}

BEGIN_MESSAGE_MAP(CiotpowerccResourceMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_TRAY_ICON, OnTrayIcon)
	ON_MESSAGE(WM_DEVICECHANGE, &CiotpowerccResourceMonitorDlg::OnDeviceChange)
	ON_BN_CLICKED(IDC_Apply, &CiotpowerccResourceMonitorDlg::OnBnClickedApply)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_COMMAND(ID_RCMENU_ShowWindow, &CiotpowerccResourceMonitorDlg::OnRcmenuShowwindow)
	ON_COMMAND(ID_RCMENU_Quit, &CiotpowerccResourceMonitorDlg::OnRcmenuQuit)
END_MESSAGE_MAP()


// CiotpowerccResourceMonitorDlg 消息处理程序

BOOL CiotpowerccResourceMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	mGpuMonitorLib.InitializeGpuMonitor();
	InitGpuList();

	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = m_hWnd;
	m_nid.uID = 1; // 唯一的图标ID
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_nid.uCallbackMessage = WM_TRAY_ICON;
	m_nid.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//lstrcpyn(m_nid.szTip, _T("Iot Power CC Resource Manager"), sizeof(m_nid.szTip));
	_tcscpy_s(m_nid.szTip, _countof(m_nid.szTip), _T("Iot Power CC Resource Manager"));

	Shell_NotifyIcon(NIM_ADD, &m_nid);

	bool isEjected;
	if (!RefreshIotPowerCCDevice(isEjected))
	{
		//Error
	}

	//Pre allocate
	//Header
	pDataSend[0] = 0x55;
	pDataSend[1] = 0xaa;
	pDataSend[2] = 0x40;
	pDataSend[3] = 0xf0;
	//Ender
	pDataSend[62] = 0x0a;
	pDataSend[63] = 0x0d;

	//Default UI
	m_combo_refreshinterval.SetCurSel(1);//1s
	//TODO:GPUDevice


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CiotpowerccResourceMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CiotpowerccResourceMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CiotpowerccResourceMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CiotpowerccResourceMonitorDlg::SplitStr(CString strSrc, CString strGap, CStringArray& strResult)
{
	int nPos = strSrc.Find(strGap);
	CString strLeft = _T("");

	while (0 <= nPos)
	{
		strLeft = strSrc.Left(nPos);
		if (!strLeft.IsEmpty())
		{
			strResult.Add(strLeft);
		}

		strSrc = strSrc.Right(strSrc.GetLength() - nPos - strGap.GetLength());
		nPos = strSrc.Find(strGap);
	}

	if (!strSrc.IsEmpty())
	{
		strResult.Add(strSrc);
	}
}

LRESULT CiotpowerccResourceMonitorDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(lParam) == WM_RBUTTONUP)
	{
		// 右键单击托盘图标时的处理
		POINT point;
		HMENU hMenu, hSubMenu;
		GetCursorPos(&point); //鼠标位置
		hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1)); //菜单的ID
		hSubMenu = GetSubMenu(hMenu, 0);
		SetForegroundWindow();

		TrackPopupMenu(hSubMenu, 0,
			point.x + 1, point.y + 1, 0, m_hWnd, NULL);
	}
	else if (LOWORD(lParam) == WM_LBUTTONUP)
	{
		// 左键单击托盘图标时的处理
		ShowWindow(SW_NORMAL);
	}

	return 0;
}


LRESULT CiotpowerccResourceMonitorDlg::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	//Avoid jitter
	KillTimer(0);
	SetTimer(0, 500, NULL);

	return 0;
}

bool CiotpowerccResourceMonitorDlg::RefreshIotPowerCCDevice(bool& isDeviceEject)
{
	//const DWORD vid = 0x1209;
	//const DWORD pid = 0x7301;

	isDeviceEject = true;
	iotpowercc_list.clear();

	HDEVINFO deviceInfoSet;
	SP_DEVINFO_DATA deviceInfoData;

	deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (deviceInfoSet == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		return false;
	}
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	int deviceIndex = 0;

	while (SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData)) 
	{
		// 获取设备信息
		TCHAR deviceName[MAX_PATH];
		if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)deviceName, sizeof(deviceName), NULL)) 
		{
			// 获取设备路径
			SP_DEVICE_INTERFACE_DATA interfaceData;
			interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			if (SetupDiEnumDeviceInterfaces(deviceInfoSet, &deviceInfoData, &GUID_DEVINTERFACE_USB_DEVICE, 0, &interfaceData))
			{
				DWORD requiredSize = 0;
				SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL);

				SP_DEVICE_INTERFACE_DETAIL_DATA* interfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)new BYTE[requiredSize];
				interfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, interfaceDetailData, requiredSize, NULL, NULL))
				{
					CString devicePath = interfaceDetailData->DevicePath;
					devicePath.MakeLower();
					// 检查设备的VID和PID是否与指定的相匹配
					if (devicePath.Find(_T("vid_1209"), 0) != -1 && devicePath.Find(_T("pid_7301"), 0) != -1) 
					{
						if (devicePath.Find(currentConnectDeviceName, 0) != -1)
						{
							isDeviceEject = false;
						}
						iotDeviceInfo deviceInfo;
						deviceInfo.deviceName = deviceName;
						deviceInfo.devicePath = devicePath;
						iotpowercc_list.push_back(deviceInfo);
					}
				}

				delete[] interfaceDetailData;
			}
		}

		deviceIndex++;
	}
	SetupDiDestroyDeviceInfoList(deviceInfoSet);


	m_combo_iotdevices.ResetContent();
	for (int i = 0; i < iotpowercc_list.size(); ++i)
	{
		CStringArray sp;
		SplitStr(iotpowercc_list[i].devicePath, L"#", sp);
		CString iotDeviceName = sp.GetAt(sp.GetSize() - 2);

		if (iotDeviceName.Find(currentConnectDeviceName, 0) != -1)
		{
			m_combo_iotdevices.SetCurSel(i);
		}

		iotDeviceName.MakeUpper();
		m_combo_iotdevices.AddString(iotDeviceName);
	}

	return true;
}

HANDLE CiotpowerccResourceMonitorDlg::OpenUSBDevice(const CString& devicePath)
{
	HANDLE hDevice = CreateFile(devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (hDevice == INVALID_HANDLE_VALUE) {
		// 打开设备失败
		TRACE(_T("Failed to open USB device: %d\n"), GetLastError());
		return NULL;
	}
	return hDevice;
}

BOOL CiotpowerccResourceMonitorDlg::QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hDeviceHandle, PIPE_ID* pipeid)
{
	if (hDeviceHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;

	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
	ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

	WINUSB_PIPE_INFORMATION  Pipe;
	ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));


	bResult = WinUsb_QueryInterfaceSettings(hDeviceHandle, 0, &InterfaceDescriptor);
	
	if (bResult)
	{
		for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
		{
			bResult = WinUsb_QueryPipe(hDeviceHandle, 0, index, &Pipe);

			if (bResult)
			{
				if (Pipe.PipeType == UsbdPipeTypeControl)
				{
					//printf("Endpoint index: %d Pipe type: Control Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
				}
				if (Pipe.PipeType == UsbdPipeTypeIsochronous)
				{
					//printf("Endpoint index: %d Pipe type: Isochronous Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
				}
				if (Pipe.PipeType == UsbdPipeTypeBulk)
				{
					if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
					{
						//printf("Endpoint index: %d Pipe type: Bulk Pipe ID: %c.\n", index, Pipe.PipeType, Pipe.PipeId);
						pipeid->PipeInId = Pipe.PipeId;
					}
					if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
					{
						//printf("Endpoint index: %d Pipe type: Bulk Pipe ID: %c.\n", index, Pipe.PipeType, Pipe.PipeId);
						pipeid->PipeOutId = Pipe.PipeId;
					}

				}
				if (Pipe.PipeType == UsbdPipeTypeInterrupt)
				{
					//printf("Endpoint index: %d Pipe type: Interrupt Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
				}
			}
			else
			{
				continue;
			}
		}
	}

	return bResult;
}

void CiotpowerccResourceMonitorDlg::OnBnClickedApply()
{
	// TODO: 在此添加控件通知处理程序代码
	if (isConnected)
	{
		//Disconnect

		//Stop timer or thread first
		//KillTimer(10);
		isConnected = false;

		//Free memory & release handle
		WinUsb_Free(iotDeviceWinUSBHandle);
		CloseHandle(iotDeviceHandle);

		currentConnectDeviceName = L"";

		//UI
		GetDlgItem(IDC_Combo_iotDevices)->EnableWindow(TRUE);
		GetDlgItem(IDC_Combo_GPUs)->EnableWindow(TRUE);
		GetDlgItem(IDC_Combo_RefreshInterval)->EnableWindow(TRUE);

		GetDlgItem(IDC_Apply)->SetWindowText(L"应用修改并启动！！");

	}
	else
	{
		//Connect
		int device_select = m_combo_iotdevices.GetCurSel();
		if (device_select < 0)
		{
			//No Device
			return;
		}

		iotDeviceHandle = OpenUSBDevice(iotpowercc_list[device_select].devicePath);
		if (iotDeviceHandle == NULL)
		{
			//Open failed
			DWORD er = GetLastError();
			return;
		}

		if (WinUsb_Initialize(iotDeviceHandle, &iotDeviceWinUSBHandle))
		{
			// WinUSB 初始化成功
		}
		else 
		{
			DWORD error = GetLastError();
			return;
		}

		bool bResult = QueryDeviceEndpoints(iotDeviceWinUSBHandle, &iotDeviceWinUSBPipeID);
		if (!bResult)
		{
			DWORD error = GetLastError();
			return;
		}

		int interval_select = m_combo_refreshinterval.GetCurSel();
		if (interval_select < 0)
		{
			//interval undefined(impossible XD)
			return;
		}

		//Record connected device name
		GetDlgItemText(IDC_Combo_iotDevices, currentConnectDeviceName);
		currentConnectDeviceName.MakeLower();

		//Start timer or Thread to send data to iot cc
		//SetTimer(10, time_interval_list[interval_select], NULL);
		AfxBeginThread(ThreadRefreshData, this);

		//UI
		GetDlgItem(IDC_Combo_iotDevices)->EnableWindow(FALSE);
		GetDlgItem(IDC_Combo_GPUs)->EnableWindow(FALSE);
		GetDlgItem(IDC_Combo_RefreshInterval)->EnableWindow(FALSE);

		GetDlgItem(IDC_Apply)->SetWindowText(L"停止");

		isConnected = true;
		ShowWindow(SW_HIDE);
	}
}

bool CiotpowerccResourceMonitorDlg::SendDataToDevice()
{
	CString tim;
	
	//Get system time
	SYSTEMTIME st;
	GetLocalTime(&st);

	pDataSend[7] = static_cast<BYTE>(st.wYear % 0x100);
	pDataSend[8] = static_cast<BYTE>(st.wYear / 0x100);
	pDataSend[9] = st.wMonth;
	pDataSend[10] = st.wDay;
	pDataSend[11] = st.wHour;
	pDataSend[12] = st.wMinute;
	
	//Debug
	pDataSend[4] = getCpuUsage() * 100;//CPU
	pDataSend[5] = getMemoryRate();//RAM
	pDataSend[6] = UpdateGpuUtilization();//GPU
	
	//Send
	ULONG nBytesSent = 0;
	WinUsb_WritePipe(iotDeviceWinUSBHandle, iotDeviceWinUSBPipeID.PipeOutId, pDataSend, 64, &nBytesSent, 0);

	if (nBytesSent != 64)
	{
		DWORD error = GetLastError();
		return false;
	}

	return true;
}


void CiotpowerccResourceMonitorDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (nIDEvent)
	{
	case 0:
	{
		KillTimer(0);

		bool isEjected;
		if (!RefreshIotPowerCCDevice(isEjected))
		{
			//Error
			return;
		}

		if (isEjected && isConnected)
		{
			//Stop timer or terminate thread then reset state

			OnBnClickedApply();//temporary

		}
		break;
	}
	}

	CDialogEx::OnTimer(nIDEvent);
}

__int64 CiotpowerccResourceMonitorDlg::Filetime2Int64(const FILETIME& ftime)
{
	LARGE_INTEGER li;
	li.LowPart = ftime.dwLowDateTime;
	li.HighPart = ftime.dwHighDateTime;
	return li.QuadPart;
}

__int64 CiotpowerccResourceMonitorDlg::CompareFileTime2(const FILETIME& preTime, const FILETIME& nowTime)
{
	return Filetime2Int64(nowTime) - Filetime2Int64(preTime);
}

double CiotpowerccResourceMonitorDlg::getCpuUsage()
{
	FILETIME preIdleTime;
	FILETIME preKernelTime;
	FILETIME preUserTime;
	GetSystemTimes(&preIdleTime, &preKernelTime, &preUserTime);

	Sleep(100);

	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;
	GetSystemTimes(&idleTime, &kernelTime, &userTime);

	auto idle = CompareFileTime2(preIdleTime, idleTime);
	auto kernel = CompareFileTime2(preKernelTime, kernelTime);
	auto user = CompareFileTime2(preUserTime, userTime);

	if (kernel + user == 0)
		return 0;

	return 1.0 * (kernel + user - idle) / (kernel + user);
}

DWORD CiotpowerccResourceMonitorDlg::getMemoryRate()
{
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(memStatus);
	GlobalMemoryStatusEx(&memStatus);
	return memStatus.dwMemoryLoad;
}

/*
bool CiotpowerccResourceMonitorDlg::GetDeviceDescription(CString devicePath, CString& deviceDesc)
{
	devicePath.MakeUpper();
	devicePath.Replace(L"#", L"\\");

	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	GUID GUID_DISPLAY_DEVICE_ARRIVAL = { 0x1CA05180, 0xA699, 0x450A, 0x9A, 0x0C, 0xDE, 0x4F, 0xBE, 0x3D, 0xDD, 0x89 };
	hDevInfo = SetupDiGetClassDevs(&GUID_DISPLAY_DEVICE_ARRIVAL, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (hDevInfo == INVALID_HANDLE_VALUE) 
	{
		//_T("Error: Unable to enumerate devices.");
		return false;
	}

	// 遍历所有设备
	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i)
	{
		TCHAR deviceName[MAX_PATH];
		if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)deviceName, sizeof(deviceName), NULL))
		{
			// 检查设备路径是否匹配
			if (devicePath.Find(CString(deviceName)) != -1)
			{
				// 获取设备描述
				TCHAR device_Desc[1024];
				if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)device_Desc, sizeof(device_Desc), NULL)) 
				{
					SetupDiDestroyDeviceInfoList(hDevInfo);
					deviceDesc = CString(device_Desc);
					return true; 
				}
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return false;// _T("Device not found.");
}
*/

void CiotpowerccResourceMonitorDlg::InitGpuList()
{
	int gpuCount = mGpuMonitorLib.GpuCount();

	mGpuList.clear();

	for (int i = 0; i < gpuCount; ++i)
	{
		mGpuList.emplace_back(mGpuMonitorLib.GetGpuName(i).c_str());
	}

	m_combo_gpus.ResetContent();
	for (int i = 0; i < mGpuList.size(); i++)
	{
		m_combo_gpus.AddString(mGpuList[i]);
	}
	m_combo_gpus.AddString(L"Auto: Maximum");
	m_combo_gpus.AddString(L"Auto: Average");
	m_combo_gpus.SetCurSel(mGpuList.size());
}

double CiotpowerccResourceMonitorDlg::UpdateGpuUtilization()
{
	mGpuMonitorLib.UpdatePerfDatas();

	std::vector<float> gpuUtilList;
	gpuUtilList.resize(mGpuList.size(), 0.f);

	for (int i = 0; i < mGpuList.size(); ++i)
	{
		gpuUtilList[i] = mGpuMonitorLib.GetGpuUtilization(i);
	}

	double utilization = 0.f;
	int gpu_selected = m_combo_gpus.GetCurSel();
	if (gpu_selected < mGpuList.size() && gpu_selected >= 0)
	{
		utilization = gpuUtilList[gpu_selected];
	}
	else if (gpu_selected >= mGpuList.size())
	{
		for (int i = 0; i < mGpuList.size(); ++i)
		{
			if (gpu_selected == mGpuList.size())//Auto: Maximum
			{
				utilization = *std::max_element(gpuUtilList.begin(), gpuUtilList.end());
			}
			else if(gpu_selected == mGpuList.size() + 1)//Auto: Average
			{
				float peakTotal = std::accumulate(gpuUtilList.begin(), gpuUtilList.end(), 0.0f);
				utilization = peakTotal / gpuUtilList.size();
			}
		}
	}

	return utilization;
}

UINT CiotpowerccResourceMonitorDlg::ThreadRefreshData(LPVOID lpParam)
{
	CiotpowerccResourceMonitorDlg* pThis = (CiotpowerccResourceMonitorDlg*)lpParam;

	int refresh_interval = pThis->time_interval_list[pThis->m_combo_refreshinterval.GetCurSel()];//ms

	while (pThis->isConnected)
	{
		Sleep(refresh_interval);

		pThis->SendDataToDevice();
	}

	return 0;
}

void CiotpowerccResourceMonitorDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	BOOL bShift = (GetKeyState(VK_SHIFT) & 0x8000);
	if(bShift)
		CDialogEx::OnClose();
	else
		ShowWindow(SW_HIDE);
}


void CiotpowerccResourceMonitorDlg::OnRcmenuShowwindow()
{
	// TODO: 在此添加命令处理程序代码
	ShowWindow(SW_NORMAL);
}


void CiotpowerccResourceMonitorDlg::OnRcmenuQuit()
{
	// TODO: 在此添加命令处理程序代码
	EndDialog(IDOK);
}
