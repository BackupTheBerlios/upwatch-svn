// MonitorClient.cpp : Implementation of WinMain

#include "stdafx.h"
#include "resource.h"
#include "MonitorClient.h"
#include "SetupDialog.h"

class CMonitorClientModule : public CAtlExeModuleT< CMonitorClientModule > {};

CMonitorClientModule _AtlModule;

//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
                                LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	CSetupDialog dialog;
	dialog.DoModal();
	return _AtlModule.WinMain(nShowCmd);
}
