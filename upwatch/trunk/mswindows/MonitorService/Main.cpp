#include "main.h"
#include "monitoroutput.h"

#include <algorithm>

#define MONITOR_SERVICE_NAME TEXT("WindowsMonitorService")
#define UPWATCH_SERVICE_NAME TEXT("UpwatchService")

// Spawn thread listening the monitor status
static HANDLE hThread[2] = {0};

// Service handler
static SERVICE_STATUS_HANDLE hService = NULL;

// Main Service Routine
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);

// Upwatch service routine
void WINAPI UpwatchMain(DWORD dwArgc, LPTSTR* lpszArgv);

// Service Handler
DWORD WINAPI ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

// Upwatch Service Handler
DWORD WINAPI UpWatchServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPTSTR lpCmdLine, int nShowCmd)
{
	if (__argc == 2)
	{
		SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		// Check if install is required
		if (std::string(__argv[1]).compare("/install") == 0)
		{
			TCHAR strFileName[0x512] = {0};
			strFileName[0] = '\"';
			strFileName[GetModuleFileName(NULL, &strFileName[1], 0x512) + 1] = '\"';
			SC_HANDLE hService = CreateService(hSCM, MONITOR_SERVICE_NAME, NULL, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
				SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, strFileName, NULL, NULL, NULL, NULL, NULL);
			CloseServiceHandle(hService);
			hService = CreateService(hSCM, UPWATCH_SERVICE_NAME, NULL, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
				SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, strFileName, NULL, NULL, NULL, NULL, NULL);
			CloseServiceHandle(hService);
		}
		// Check if uninstall is required
		else if (std::string(__argv[1]).compare("/uninstall") == 0)
		{
			SC_HANDLE hService = OpenService(hSCM, MONITOR_SERVICE_NAME, SERVICE_ALL_ACCESS);
			DeleteService(hService);
			hService = OpenService(hSCM, UPWATCH_SERVICE_NAME, SERVICE_ALL_ACCESS);
			DeleteService(hService);
		}
		CloseServiceHandle(hSCM);
		return 0;
	}
	// Load the default service routine
	SERVICE_TABLE_ENTRY service_table[] =
	{
		{
            MONITOR_SERVICE_NAME,
			&ServiceMain
		},
		{
			UPWATCH_SERVICE_NAME,
			&UpwatchMain,
		},
		{ NULL, NULL }
	};
	StartServiceCtrlDispatcher(service_table);
	return 0;
}

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Load the default service routine
	std::string strDemandService(lpszArgv[0]);
	std::string strService(MONITOR_SERVICE_NAME);
	transform(strDemandService.begin(), strDemandService.end(), strDemandService.begin(), tolower);
	transform(strService.begin(), strService.end(), strService.begin(), tolower);
	// Check the right service request
	if (strDemandService != strService)
		return UpwatchMain(dwArgc, lpszArgv);
	hService = RegisterServiceCtrlHandlerEx(MONITOR_SERVICE_NAME, &ServiceHandler, NULL);
	SERVICE_STATUS ServiceStatus = {0};
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;

	// Start the service
	SetServiceStatus(hService, &ServiceStatus);
	HANDLE hEvent[2];
	hThread[0] = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&CMonitorOutput::StartMonitor),
		NULL, 0, NULL);
	CMonitorOutput::m_hEvent[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvent[0] = hThread[0];
	hEvent[1] = CMonitorOutput::m_hEvent[0];
	if (WaitForMultipleObjects(2, hEvent, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	else
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(hService, &ServiceStatus);
	return;
}

void WINAPI UpwatchMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	std::string strDemandService(lpszArgv[0]);
	std::string strService(UPWATCH_SERVICE_NAME);
	transform(strDemandService.begin(), strDemandService.end(), strDemandService.begin(), tolower);
	transform(strService.begin(), strService.end(), strService.begin(), tolower);
	// Sanity Check to see if service request is right
	if (strDemandService != strService)
		return;
	hService = RegisterServiceCtrlHandlerEx(UPWATCH_SERVICE_NAME, &UpWatchServiceHandler, NULL);
	SERVICE_STATUS ServiceStatus = {0};
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	SetServiceStatus(hService, &ServiceStatus);
	HANDLE hEvent[2];
	hThread[1] = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&CMonitorOutput::StartUpwatch),
		NULL, 0, NULL);
	CMonitorOutput::m_hEvent[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvent[0] = hThread[1];
	hEvent[1] = CMonitorOutput::m_hEvent[1];
	if (WaitForMultipleObjects(2, hEvent, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	else
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(hService, &ServiceStatus);
	return;
}

// This is to see if the service require stopping and pausing and to service status to
// the service manager
DWORD WINAPI ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	SERVICE_STATUS ServiceStatus = {0};
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
	switch (dwControl)
	{
	case SERVICE_CONTROL_CONTINUE:
		if (CMonitorOutput::m_control == FALSE)
			break;
		CMonitorOutput::m_control = FALSE;
		ResumeThread(hThread[0]);
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_PAUSE:
		if (CMonitorOutput::m_control == TRUE)
		{
			ServiceStatus.dwCurrentState = SERVICE_PAUSED;
			break;
		}	
		ServiceStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
		SetServiceStatus(hService, &ServiceStatus);
		CMonitorOutput::m_control = TRUE;
		SetEvent(CMonitorOutput::m_hEvent[0]);
		Sleep(100);
		WaitForSingleObject(CMonitorOutput::m_hEvent[0], INFINITE);
		ServiceStatus.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_STOP:
		ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(hService, &ServiceStatus);
		CMonitorOutput::m_control = 2;
		if (CMonitorOutput::m_control == TRUE)
		{
			ResumeThread(hThread[0]);
            SetEvent(CMonitorOutput::m_hEvent[0]);
		}
		WaitForSingleObject(hThread[0], INFINITE);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	default:
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	SetServiceStatus(hService, &ServiceStatus);
	return NO_ERROR;
}

DWORD WINAPI UpWatchServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	SERVICE_STATUS ServiceStatus = {0};
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
		ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(hService, &ServiceStatus);
		SetEvent(CMonitorOutput::m_hEvent[1]);
		WaitForSingleObject(hThread[1], INFINITE);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	default:
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	SetServiceStatus(hService, &ServiceStatus);
	return NO_ERROR;
}
