// SetupDialog.h : Declaration of the CSetupDialog

#pragma once

#include "resource.h"       // main symbols
#include <atlhost.h>

#include <string>
// CSetupDialog

#define WM_ICONNOTIFY WM_APP

class CSetupDialog : 
	public CDialogImpl<CSetupDialog>
{
public:
	CSetupDialog();
	~CSetupDialog();

	enum { IDD = IDD_SETUPDIALOG };

BEGIN_MSG_MAP(CSetupDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	MESSAGE_HANDLER(WM_ICONNOTIFY, OnIconNotify)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
	COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
	COMMAND_HANDLER(IDC_TESTCONNECTION, BN_CLICKED, OnBnClickedTestconnection)
END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnIconNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	static void StartMoniter(CSetupDialog* p);
	static std::string LoadData();
	static std::string GetTagData(const std::string& szData, const std::string& strTag);
	static std::string SetTagData(const std::string& strTag, const std::string& szTagData);

	static char *m_tags[];
private:
	void SaveData();

private:
	HANDLE	m_hThread;
public:
	LRESULT OnBnClickedTestconnection(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


