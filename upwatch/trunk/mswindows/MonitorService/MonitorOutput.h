#pragma once

#include <sstream>
#include <string>

class CSystemInformation;

class CMonitorOutput
{
public:
	// Main setup of monitor service
	static HRESULT	SetupMonitor(CSystemInformation* pSysInfo);

	// The monitor main thread
	static void CALLBACK StartMonitor();

	// The upwatch main thread
	static void CALLBACK StartUpwatch();

	// Spool writing routine
	static void WriteSpool(std::string& strData);

	// Get the configuration data
	std::string GetData() { return m_data.str(); }

	// Initialize the log file
	void InitLogFile();
	// End using the log file
	void EndLogFile();

	// Retrieve system status
	void SystemStat();
	// Retrieve hardware status
	void HardwareInfo();
	// Retrieve process list status
	void ProcessList(int);
	// Retrieve log status
	void ErrorLog();
	// Retrieve disk free information
	void DiskFree();

	static HANDLE				m_hEvent[2];
	static BOOL					m_control;
private:
	std::stringstream	m_data;

	static CSystemInformation*	m_pSysInfo;
	// The WMI command to retrieve certain device info
	static ULONG m_memory,	m_cpu_counter, m_disk, m_cpu, m_fan, m_temp, m_process;

	static std::string			m_domain;
	static std::string			m_server;
	static std::string			m_fromhost;
	static std::string			m_ipaddress;
	static std::string			m_date;
	static std::string			m_expires;
	static std::string			m_interval;
	static std::string			m_threshold;
	static std::string			m_yellow;
	static std::string			m_red;
	static std::string			m_spoolInterval;

	static std::string LoadData();
	static std::string GetTagData(const std::string& szData, const std::string& strTag);
	static std::string SetTagData(const std::string& strTag, const std::string& szTagData);

	static char *m_tags[];
};
