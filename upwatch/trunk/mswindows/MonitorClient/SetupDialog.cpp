// SetupDialog.cpp : Implementation of CSetupDialog

#define CONFIG_FILE "c:\\Program Files\\upwatch\\etc\\uw_sysstat.conf"

#include "stdafx.h"
#include ".\setupdialog.h"
#include ".\loguploader.h"

#include <string>
#include <sstream>
using namespace std;

// CSetupDialog


char* CSetupDialog::m_tags[] =
{
	"login", "password", "domain", "ip", "serverid", "expire", "interval",
	"threshold", "yellow", "red", "filter", "uploadsite", "dns", "upwatch"
};

template <class T>
__inline bool from_string(T &t, 
                 const std::string &s, 
                 std::ios_base & (*f)(std::ios_base&))
{
   std::istringstream iss(s);
   return !(iss>>f>>t).fail();
}

CSetupDialog::CSetupDialog()
{
}

CSetupDialog::~CSetupDialog()
{
}

LRESULT CSetupDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	string szData = LoadData();
	for (int i = 0;i < 14;i++)
		SetDlgItemText(IDC_LOGIN + i, GetTagData(szData, string(m_tags[i])).c_str());
	/*NOTIFYICONDATA nd;
	ZeroMemory(&nd, sizeof(NOTIFYICONDATA));
	nd.cbSize = sizeof(NOTIFYICONDATA);
	nd.hWnd = m_hWnd;
	nd.uID = 0;
	nd.uCallbackMessage = WM_ICONNOTIFY;
	nd.uFlags = NIF_MESSAGE | NIF_ICON;
	nd.hIcon =reinterpret_cast<HICON>(LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MONITOR), 
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
	Shell_NotifyIcon(NIM_ADD, &nd);
	//m_hThread = CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(&StartMoniter), this, 0, NULL);*/
	return 1;  // Let the system set the focus
}

LRESULT CSetupDialog::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	SaveData();
	EndDialog(IDOK);
	return 0;
}

LRESULT CSetupDialog::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT CSetupDialog::OnIconNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	/*switch (lParam)
	{
	case WM_RBUTTONDOWN:
		HMENU hMenu = CreatePopupMenu();
		MENUITEMINFO item;
		ZeroMemory(&item, sizeof(MENUITEMINFO));
		item.cbSize = sizeof(MENUITEMINFO);
		item.fMask = MIIM_STRING | MIIM_ID;

		for (int i = 0;i < 3;i++)
		{
			string str;
            switch (i)
			{
			case 0:
				if (CMonitorOutput::m_control == FALSE)
                    str = "Pause Monitor";
				else
					str = "Resume Monitor";
				break;
			case 1:
				str = "Setting";
				break;
			case 2:
				str = "Shutdown";
				break;
			}
			item.wID++;
			item.dwTypeData = const_cast<LPTSTR>(str.c_str());
			item.cch = static_cast<DWORD>(str.size());
			InsertMenuItem(hMenu, i, TRUE, &item);
		}

		SetForegroundWindow(m_hWnd);
		POINT pt;
		GetCursorPos(&pt);
		int id = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON, 
			pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hMenu);

		switch (id)
		{
		case 1:
			CMonitorOutput::m_control = 1 - CMonitorOutput::m_control;
			if (CMonitorOutput::m_control == FALSE)
				ResumeThread(m_hThread);
			break;
		case 2:
			{
                ShowWindow(SW_SHOW);
				string szData = LoadData();
				for (int i = 0;i < 13;i++)
					SetDlgItemText(IDC_LOGIN + i, GetTagData(szData, string(m_tags[i])).c_str());
			}
			break;
		case 3:
			CMonitorOutput::m_control = 2;
			WaitForSingleObject(m_hThread, 5000);
			CloseHandle(m_hThread);
			EndDialog(0);
            break;
		}
	}*/
	return 0;
}

LRESULT CSetupDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 0;
}

string CSetupDialog::LoadData()
{
	HANDLE hFile = CreateFile(CONFIG_FILE, GENERIC_READ, 0, NULL, OPEN_ALWAYS, 0, NULL);
	DWORD size = GetFileSize(hFile, NULL);
	LPTSTR data = new TCHAR[size];
	ReadFile(hFile, data, size, &size, NULL);
	CloseHandle(hFile);

	return data;
}

void CSetupDialog::SaveData()
{
	string szData = "<config>\n";
	for (DWORD i = 0;i < 14;i++)
	{
		TCHAR str[512];
        GetDlgItemText(IDC_LOGIN + i, str, 512);
		szData += "\t" + SetTagData(string(m_tags[i]), string(str)) + "\n";
	}
	szData += "</config>";
	HANDLE hFile = CreateFile(CONFIG_FILE, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	WriteFile(hFile, szData.c_str(), static_cast<DWORD>(szData.size()), &i, NULL);
	SetEndOfFile(hFile);
	CloseHandle(hFile);
}

string CSetupDialog::SetTagData(const string& strTag, const string& szTagData)
{
	return "<" + strTag + ">" + szTagData + "</" + strTag + ">";
}

string CSetupDialog::GetTagData(const string& szData, const string& strTag)
{
	string::size_type pos = szData.find("<" + strTag);
	if (pos == -1)
		return string("");
	pos = szData.find(">", pos) + 1;
	string data = szData.substr(pos, szData.find("</" + strTag + ">") - pos);
	return data;
}
LRESULT CSetupDialog::OnBnClickedTestconnection(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	char text[0x512];
	string username, password, uploadsite;
	GetDlgItemText(IDC_UPLOADSITE, text, 0x512);
	uploadsite = text;
	GetDlgItemText(IDC_LOGIN, text, 0x512);
	username = text;
	GetDlgItemText(IDC_PASSWORD, text, 0x512);
	password = text;
	string::size_type pos = uploadsite.find(":");
	WORD port;
	if (pos != -1)
		from_string(port, uploadsite.substr(pos + 1), dec);
	else
		return 0;
	CLogUploader* pUploader = CLogUploader::GetUploader();
	pUploader->SetupUploaderInfo(username, password, uploadsite.substr(0, pos), port);
	if (pUploader->Login() == 0)
		SetDlgItemText(IDC_STATUS, _T("Connect Success"));
	else
		SetDlgItemText(IDC_STATUS, _T("Connect Failed"));
	pUploader->Quit();
	pUploader->ShutdownUploader();

	return 0;
}
