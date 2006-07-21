#include ".\systeminformation.h"

#include <algorithm>
#include <functional>

CSystemInformation* CSystemInformation::_SystemInformation = NULL;

// Initializing WMI and return the pointer
HRESULT CSystemInformation::GetSystemInformation(CSystemInformation** ppSysInfo)
{
	HRESULT hr;
	if (!_SystemInformation)
	{
		_SystemInformation = new CSystemInformation();
		hr = _SystemInformation->m_hrStatus;
		if (FAILED(hr))
            _SystemInformation->Shutdown();
	}
	*ppSysInfo = _SystemInformation;
	return hr;
}

// Setup WMI
CSystemInformation::CSystemInformation()
:m_pWbemLocator(NULL), m_pWbemServices(NULL), m_bInstanceCreated(FALSE)
{
	m_hrStatus = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(m_hrStatus))
		return;
	m_hrStatus = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(m_hrStatus))
		return;
	m_hrStatus = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&m_pWbemLocator));
	if (FAILED(m_hrStatus))
		return;
	m_hrStatus = m_pWbemLocator->ConnectServer(CComBSTR(L"\\\\.\\ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &m_pWbemServices);
	if (FAILED(m_hrStatus))
		return;
	m_hrStatus = CoSetProxyBlanket(m_pWbemServices, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED(m_hrStatus))
		return;
	m_hrStatus = CoCreateInstance(CLSID_WbemRefresher, NULL, CLSCTX_SERVER, IID_IWbemRefresher, reinterpret_cast<LPVOID*>(&m_pWbemRefresher));
	if (FAILED(m_hrStatus))
		return;
	m_pWbemConfigureRefresher = CComQIPtr<IWbemConfigureRefresher>(m_pWbemRefresher);
}

CSystemInformation::~CSystemInformation()
{
	_SystemInformation = NULL;
}

void CSystemInformation::Shutdown()
{
	delete _SystemInformation;
	CoUninitialize();
}

// Add WMI queries into db
ULONG CSystemInformation::AddQuery(const BSTR strQuery)
{
	m_bstrQueries[0].push_back(strQuery);
	return static_cast<ULONG>(m_bstrQueries[0].size() - 1);
}

// Add WMI performance request into db
ULONG CSystemInformation::AddPerfMon(LPCWSTR wszClassName)
{
	m_bstrQueries[1].push_back(wszClassName);
	return static_cast<ULONG>(m_bstrQueries[1].size() - 1);
}

// Refresh the current instance of WMI and store the results
HRESULT	CSystemInformation::RefreshInstances()
{
	if (FAILED(RefreshPerfMonInstances()))
		return m_hrStatus;
	return RefreshClassesInstances();
}

// Class WMI refreshes
HRESULT	CSystemInformation::RefreshClassesInstances()
{
	std::vector<std::wstring>::const_iterator iter = m_bstrQueries[0].begin();
	ULONG size = static_cast<ULONG>(m_bstrQueries[0].size());
	m_ppClassObject.clear();
	for (int i = 0;iter != m_bstrQueries[0].end();iter++, i++)
	{
		CComPtr<IEnumWbemClassObject> pEnumObjects;
		m_hrStatus = m_pWbemServices->ExecQuery(CComBSTR(L"WQL"), CComBSTR(iter->c_str()), 
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_ENSURE_LOCATABLE, NULL, &pEnumObjects);
		if (FAILED(m_hrStatus))
			break;
		ULONG j = 0, returned;
		std::vector<CComPtr<IWbemClassObject> >pObjects(20);
NextSet:
		m_hrStatus = pEnumObjects->Next(INFINITE, 20, &pObjects[j], &returned);
		j += 20;
		if (FAILED(m_hrStatus))
			break;
		else if (m_hrStatus == WBEM_S_NO_ERROR)
		{
			pObjects.resize(j);
			goto NextSet;
		}
		else if (returned > 0)
		{
            pObjects.erase(pObjects.begin() + j - 20 + returned, pObjects.end());
			m_ppClassObject.push_back(pObjects);
		}
	}
	return m_hrStatus;
}

// PerfMon refreshes
HRESULT CSystemInformation::RefreshPerfMonInstances()
{
	ULONG size = static_cast<ULONG>(m_bstrQueries[1].size());
	if (!m_bInstanceCreated)
	{
		std::vector<std::wstring>::const_iterator iter = m_bstrQueries[1].begin();
		for (int i = 0;iter != m_bstrQueries[1].end();iter++)
		{
			CComPtr<IWbemHiPerfEnum> p;
			long id;
			m_hrStatus = m_pWbemConfigureRefresher->AddEnum(m_pWbemServices, iter->c_str(), 0, NULL, &p, &id);
			if (FAILED(m_hrStatus))
				return m_hrStatus;
			m_lId.push_back(id);
			m_ppWbemHiPerfEnum.push_back(p);
		}
	}
	m_hrStatus = m_pWbemRefresher->Refresh(WBEM_FLAG_REFRESH_AUTO_RECONNECT);
	if (FAILED(m_hrStatus))
		return m_hrStatus;
	m_ppObjectAccess.clear();
	m_ppObjectAccess.resize(size);
	for (unsigned i = 0;i < size;i++)
	{
		ULONG returned;
        m_hrStatus = m_ppWbemHiPerfEnum[i]->GetObjects(0, 0, NULL, &returned);
		if (m_hrStatus == WBEM_E_BUFFER_TOO_SMALL)
		{
			m_ppObjectAccess[i].resize(returned);
			m_hrStatus = m_ppWbemHiPerfEnum[i]->GetObjects(0, returned, &m_ppObjectAccess[i][0], &returned);
			if (SUCCEEDED(m_hrStatus))
				continue;
		}
		break;
	}
	if (SUCCEEDED(m_hrStatus))
		m_bInstanceCreated = TRUE;
	return m_hrStatus;
}

// Send the queries to WMI
HRESULT CSystemInformation::Query(LPCWSTR wszQuery, std::vector<std::vector<CComVariant> >& strPropertyCollections)
{
	CComPtr<IEnumWbemClassObject> pEnumObjects;
	HRESULT m_hrStatus = m_pWbemServices->ExecQuery(CComBSTR(L"WQL"), CComBSTR(wszQuery), 
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_ENSURE_LOCATABLE, NULL, &pEnumObjects);
	if (FAILED(m_hrStatus))
		return m_hrStatus;
	ULONG j = 0, returned;
	m_ppAdhocClassObject.clear();
	m_ppAdhocClassObject.resize(20);
	while (1)
	{
        m_hrStatus = pEnumObjects->Next(INFINITE, 20, &m_ppAdhocClassObject[j], &returned);
		j += 20;
		if (FAILED(m_hrStatus))
			return m_hrStatus;
		else if (m_hrStatus == WBEM_S_NO_ERROR)
		{
			m_ppAdhocClassObject.resize(j);
			continue;
		}
		else if (returned > 0)
		{
			m_ppAdhocClassObject.erase(m_ppAdhocClassObject.begin() + j - 20 + returned, m_ppAdhocClassObject.end());
			break;
		}
	}
	return m_hrStatus;
}

// Retrieve the result from our refreshes WMI by using properties names
HRESULT CSystemInformation::GetResult(LPCWSTR wszName, std::vector<CComVariant> &strProperties)
{
	std::vector<CComPtr<IWbemClassObject> >::const_iterator iter = m_ppAdhocClassObject.begin();
	for (;iter != m_ppAdhocClassObject.end();iter++)
	{
		CComVariant Val;
		m_hrStatus = (*iter)->Get(wszName, 0, &Val, NULL, NULL);
		if (FAILED(m_hrStatus))
			break;
		strProperties.push_back(Val);
	}
	return m_hrStatus;
}

// Clear all performance data
void CSystemInformation::ClearInstances()
{
	m_bInstanceCreated = FALSE;
	m_ppObjectAccess.clear();
	m_ppClassObject.clear();
	m_ppWbemHiPerfEnum.clear();
	for (std::vector<long>::const_iterator iter = m_lId.begin();iter != m_lId.end();iter++)
		m_pWbemConfigureRefresher->Remove(*iter, 0);
	m_lId.clear();
	m_bstrQueries[0].clear();
	m_bstrQueries[1].clear();
}

// Get result by using type, index and names
HRESULT CSystemInformation::GetResult(ULONG ulType, ULONG ulIndex, LPCWSTR wszName, std::vector<CComVariant> &strProperties)
{
	strProperties.clear();
	if (ulType == 1 && ulIndex < m_ppObjectAccess.size())
	{
		for (std::vector<CComPtr<IWbemObjectAccess> >::iterator iter = m_ppObjectAccess[ulIndex].begin();
		iter != m_ppObjectAccess[ulIndex].end();iter++)
		{
			CComVariant Val;
			m_hrStatus = (*iter)->Get(wszName, 0, &Val, NULL, NULL);
			if (FAILED(m_hrStatus))
				break;
			strProperties.push_back(Val);
		}
	}
	else if (ulType == 0 && ulIndex < m_ppClassObject.size())
	{
		for (std::vector<CComPtr<IWbemClassObject> >::iterator iter = m_ppClassObject[ulIndex].begin();
		iter != m_ppClassObject[ulIndex].end();iter++)
		{
			CComVariant Val;
			m_hrStatus = (*iter)->Get(wszName, 0, &Val, NULL, NULL);
			if (FAILED(m_hrStatus))
				break;
			strProperties.push_back(Val);
		}
	}
	return m_hrStatus;
}