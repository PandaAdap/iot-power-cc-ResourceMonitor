
// iot-power-cc-ResourceMonitorDlg.h: 头文件
//

#pragma once

#include <winusb.h>
#pragma comment (lib, "winusb.lib")

#include <vector>
#include <usbiodef.h>
#include <initguid.h>
#include <setupapi.h>
#pragma comment (lib, "setupapi.lib")

#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")

#include <GpuMonitorLib.h>


#define WM_TRAY_ICON WM_USER+0x01

// CiotpowerccResourceMonitorDlg 对话框
class CiotpowerccResourceMonitorDlg : public CDialogEx
{
// 构造
public:
	CiotpowerccResourceMonitorDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CiotpowerccResourceMonitorDlg();

	//For iot Device
	struct iotDeviceInfo {
		CString deviceName;	//Device Name (Serial Number)
		CString devicePath;	//Device Path
	};

	//WinUSB comunicate pipe 
	struct PIPE_ID
	{
		UCHAR  PipeInId;
		UCHAR  PipeOutId;
	};

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IOTPOWERCCRESOURCEMONITOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:

	//Notify
	NOTIFYICONDATA m_nid;

	//Connect state
	bool isConnected = false;

	//Remember current connected device
	CString currentConnectDeviceName = L"";

	HANDLE iotDeviceHandle = NULL;	//iot Device handel
	WINUSB_INTERFACE_HANDLE iotDeviceWinUSBHandle = NULL;	//iot Device USB handle
	PIPE_ID iotDeviceWinUSBPipeID;	//iot Device USB pipe struct

	BYTE pDataSend[64] = { 0x00 };	//Data frame to send
	int time_interval_list[4] = { 400,900,1400,1900 };	//ms	(cpu util need 100ms)

	std::vector<iotDeviceInfo> iotpowercc_list;	//iot device list

	/// <summary>
	/// Detect iot device connected to PC and refresh list
	/// </summary>
	/// <param name="[Out] isDeviceEjected">: set to TRUE if connected device is ejected</param>
	/// <returns>Return TRUE if refresh successful</returns>
	bool RefreshIotPowerCCDevice(bool& isDeviceEjected);

	/// <summary>
	/// Split string by gap
	/// </summary>
	/// <param name="[In] strSrc">: String to split</param>
	/// <param name="[In] strGap">: Gap</param>
	/// <param name="[Out] strResult">: Splited string array</param>
	void SplitStr(CString strSrc, CString strGap, CStringArray& strResult);

	/// <summary>
	/// Open iot device by device path
	/// </summary>
	/// <param name="[In] devicePath">: Device path</param>
	/// <returns>Handle of device, failed if handle is NULL</returns>
	HANDLE OpenUSBDevice(const CString& devicePath);

	/// <summary>
	/// Function to send data frame to device
	/// </summary>
	/// <returns>Return FALSE if failed</returns>
	bool SendDataToDevice();

	/// <summary>
	/// Query device endpoints(USB)
	/// </summary>
	/// <param name="[In] hDeviceHandle">: USB device handle</param>
	/// <param name="[In] pipeid">: Pointer of PIPE_ID struct</param>
	/// <returns>Return FALSE if failed</returns>
	BOOL CiotpowerccResourceMonitorDlg::QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hDeviceHandle, PIPE_ID* pipeid);

	GpuMonitorLib mGpuMonitorLib;	//Gpu Monitor

	std::vector<CString> mGpuList;	//List of GPU

	/// <summary>
	/// Load all gpu adapters
	/// </summary>
	void InitGpuList();

	/// <summary>
	/// Update gpu perf counter and get current utilization
	/// </summary>
	/// <returns>Current gpu utilization in percent(%)</returns>
	double UpdateGpuUtilization();

	/// <summary>
	/// Convert time to long long for cpu utilization calc
	/// </summary>
	/// <param name="[In] ftime">: Pointer of FILETIME struct</param>
	/// <returns>__int64 time </returns>
	__int64 Filetime2Int64(const FILETIME& ftime);

	/// <summary>
	/// Compare time to get interval
	/// </summary>
	/// <param name="[In] preTime">: Time at past</param>
	/// <param name="[In] nowTime">: Time right now</param>
	/// <returns>Time interval in __int64</returns>
	__int64 CompareFileTime2(const FILETIME& preTime, const FILETIME& nowTime);

	/// <summary>
	/// Calc cpu utilization
	/// </summary>
	/// <returns>CPU utilization in percent(%)</returns>
	double getCpuUsage();

	/// <summary>
	/// Get system memory usage
	/// </summary>
	/// <returns>Memory usage in percent(%)</returns>
	DWORD getMemoryRate();

	/// <summary>
	/// Thread for refresh all perf counter
	/// </summary>
	/// <param name="[In] lpParam">: Pointer of current instance</param>
	/// <returns>Success if return 0</returns>
	static UINT ThreadRefreshData(LPVOID lpParam);


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDeviceChange(WPARAM wParam, LPARAM lParam);
	CComboBox m_combo_iotdevices;
	afx_msg void OnBnClickedApply();
	CComboBox m_combo_gpus;
	CComboBox m_combo_refreshinterval;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnRcmenuShowwindow();
	afx_msg void OnRcmenuQuit();
};
