#pragma once

#pragma comment(lib, "Wbemuuid.lib")

#define _WIN32_DCOM
#include <Wbemidl.h>
#include <atlbase.h>

#include <vector>
#include <string>

class CSystemInformation
{
public:
	//Load and get the WMI service pointer for retrieving WMI information
	static	HRESULT GetSystemInformation(CSystemInformation** ppSysInfo);
	//Shutdown WMI connection
	void	Shutdown();

	//Refresh WMI Instance for performance update
	HRESULT	RefreshInstances();
	//Clear the current performance data
	void	ClearInstances();
	//Get the current peroformance data
	HRESULT GetResult(ULONG ulType, ULONG ulIndex, LPCWSTR wszName, std::vector<CComVariant> &strProperties);
	HRESULT	GetStatus() { return m_hrStatus; }

	//Add a query into the database to retrieve the required WMI information
	ULONG	AddQuery(const BSTR strQuery);
	//Add a performance monitor to the WMI to retrieve the required performance data
	ULONG	AddPerfMon(LPCWSTR wszClassName);

	//Refresh only non-perfmon WMI queries
	HRESULT RefreshClassesInstances();
	//Refresh only WMI performance data
	HRESULT RefreshPerfMonInstances();

	HRESULT Query(LPCWSTR wszQuery, std::vector<std::vector<CComVariant> >& strPropertyCollections);
	HRESULT GetResult(LPCWSTR wszName, std::vector<CComVariant> &strProperties);

private:
	CSystemInformation();
	~CSystemInformation();

private:
	CComPtr<IWbemLocator>				m_pWbemLocator;
	CComPtr<IWbemServices>				m_pWbemServices;
	CComPtr<IWbemRefresher>				m_pWbemRefresher;
	CComPtr<IWbemConfigureRefresher>	m_pWbemConfigureRefresher;
	HRESULT								m_hrStatus;
	BOOL								m_bInstanceCreated;

	std::vector<CComPtr<IWbemHiPerfEnum> >					m_ppWbemHiPerfEnum;
	std::vector<std::vector<CComPtr<IWbemClassObject> > >	m_ppClassObject;
	std::vector<std::vector<CComPtr<IWbemObjectAccess> > >	m_ppObjectAccess;
	std::vector<CComPtr<IWbemClassObject> >					m_ppAdhocClassObject;

	std::vector<std::wstring>								m_bstrQueries[2];
	std::vector<long>										m_lId;

private:
	static CSystemInformation* _SystemInformation;
};
