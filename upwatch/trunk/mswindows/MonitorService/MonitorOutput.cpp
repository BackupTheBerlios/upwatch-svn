#pragma comment(lib, "comsupp.lib")

#include "main.h"
#include "atlsafe.h"
#include "comutil.h"
#include "Tlhelp32.h"

#include ".\loguploader.h"
#include ".\systeminformation.h"
#include ".\monitoroutput.h"
#include ".\errorlog.h"

#include <fstream>
#include <strstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <map>
#include <ctime>
using namespace std;

// The tags are in order to retrieve data from the configuration file
char* CMonitorOutput::m_tags[] =
{
	"login", "password", "domain", "ip", "serverid", "expire", "interval",
	"threshold", "yellow", "red", "filter", "uploadsite", "dns", "upwatch"
};

HANDLE CMonitorOutput::m_hEvent[2] = {0};
CSystemInformation*	CMonitorOutput::m_pSysInfo = NULL;
BOOL CMonitorOutput::m_control = FALSE;
ULONG CMonitorOutput::m_memory,	CMonitorOutput::m_cpu_counter, CMonitorOutput::m_disk, 
	CMonitorOutput::m_cpu, CMonitorOutput::m_fan, CMonitorOutput::m_temp,
	CMonitorOutput::m_process;

std::string			CMonitorOutput::m_domain;
std::string			CMonitorOutput::m_server;
std::string			CMonitorOutput::m_fromhost;
std::string			CMonitorOutput::m_ipaddress;
std::string			CMonitorOutput::m_date;
std::string			CMonitorOutput::m_expires;
std::string			CMonitorOutput::m_interval;
std::string			CMonitorOutput::m_threshold;
std::string			CMonitorOutput::m_yellow;
std::string			CMonitorOutput::m_red;
std::string			CMonitorOutput::m_spoolInterval;

template <class T>
__inline bool from_string(T &t, 
                 const std::string &s, 
                 std::ios_base & (*f)(std::ios_base&))
{
   std::istringstream iss(s);
   return !(iss>>f>>t).fail();
}

template <class T>
__inline const T from_string_1(CComVariant& s)
{
	char* str = _com_util::ConvertBSTRToString(s.bstrVal);
	std::istringstream iss(str);
	delete[] str;
	T t;
	iss>>dec>>t;
	return t;
}

template <class T>
__inline std::string to_string(T t, std::ios_base & (*f)(std::ios_base&))
{
   std::ostringstream oss;
   oss << f << t;
   return oss.str();
}

// Upwatch main thread
void CALLBACK CMonitorOutput::StartUpwatch()
{
	// Load user data
	string user, pass, uploadsite, upwatch_interval, szData;
	szData = LoadData();
	if (szData.compare("") == 0)
		return;
	// Send event to notify the manager the thread start running
	SetEvent(m_hEvent[1]);

	// Decode the data into variables
	for (int i = 0;i < 14;i++)
	{
        string strTagData = GetTagData(szData, string(m_tags[i]));
		switch (i)
		{
		case 0:
			user = strTagData;
			break;
		case 1:
			pass = strTagData;
			break;
		case 13:
			upwatch_interval = strTagData;
			break;
		case 11:
			uploadsite = strTagData;
			break;
		}
	}
	DWORD interval;
	from_string(interval, upwatch_interval, dec);
	interval *= 1000;

	string::size_type pos = uploadsite.find(":");
	WORD port;
	if (pos != -1)
		from_string(port, uploadsite.substr(pos + 1), dec);

	// Start the counter and uploader
	CLogUploader* pUploader = CLogUploader::GetUploader();
	while (WaitForSingleObject(m_hEvent[1], interval) == WAIT_TIMEOUT)
	{
		// Spool files in SPOOL_DIRECTORY
		WIN32_FIND_DATA data;
		HANDLE hFindFile = FindFirstFile(string(SPOOL_DIRECTORY + string("\\*.*")).c_str(), &data);
		if (hFindFile == INVALID_HANDLE_VALUE)
			continue;
		vector<string> files;
		do
		{
			files.push_back(data.cFileName);
		} while (FindNextFile(hFindFile, &data));
		FindClose(hFindFile);
		if (files.size() <= 2)
			continue;
		sort(files.begin(), files.end());
		pUploader->SetupUploaderInfo(user, pass, uploadsite.substr(0, pos), port);
		// Startup upwatch session
		if (files.size() && pUploader->Login() == 0)
		{
			vector<string>::const_iterator iter = files.begin();
			for (;iter != files.end();iter++)
			{
				// Retrieve the file data from each file and execute upwatch command to upload them
				string strFileName = SPOOL_DIRECTORY + string("\\") + *iter;
                HANDLE hFile = CreateFile(strFileName.c_str(), FILE_READ_ACCESS, 0, NULL,
					OPEN_EXISTING, 0, NULL);
				if (hFile == INVALID_HANDLE_VALUE)
					continue;
				DWORD dwSize = GetFileSize(hFile, NULL);
				auto_ptr<char> buffer(new char[dwSize]);
				ReadFile(hFile, buffer.get(), dwSize, &dwSize, NULL);
				CloseHandle(hFile);
				string szHead = "data " + to_string(dwSize, dec) + " " + *iter + "\n";
				if (pUploader->Command(szHead) == 0 && pUploader->Command(buffer.get(), dwSize) == 0)
                    DeleteFile(strFileName.c_str());
				else
					break;
			}
			if (iter == files.end())
				files.clear();
			// End a session
			pUploader->Quit();
		}
	}
	CLogUploader::ShutdownUploader();
}

void CALLBACK CMonitorOutput::StartMonitor()
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	string szData;
	CSystemInformation* sysInfo = NULL;
	// Get the WMI connection pointer
	HRESULT hr = CSystemInformation::GetSystemInformation(&sysInfo);
	if (sysInfo == NULL)
	{
		string str = "Failed To Initialize WMI: " + to_string(hr, hex);
		//MessageBox(NULL, str.c_str(), "Error", MB_ICONERROR);
		goto Exit;
	}
	hr = SetupMonitor(sysInfo);
	if (FAILED(hr))
	{
		string str = "Failed To Query WMI: " + to_string(hr, hex);
		//MessageBox(NULL, str.c_str(), "Error", MB_ICONERROR);
		sysInfo->Shutdown();
		goto Exit;
	}
	// Tell the service manager the service is running
	SetEvent(m_hEvent[0]);

	// Get the data from the configuration file
	szData = LoadData();
	if (szData.compare("") == 0)
		goto Exit;
	for (int i = 0, j, k;i < 13;i++)
	{
        string strTagData = GetTagData(szData, string(m_tags[i]));
		switch (i)
		{
		/*case 0:
			user = strTagData;
			break;
		case 1:
			pass = strTagData;
			break;*/
		case 2:
			m_domain = strTagData;
			break;
		case 3:
			m_ipaddress = strTagData;
			break;
		case 4:
			m_server = strTagData;
			break;
		case 5:
			from_string(j, strTagData, dec);		// expires
			break;
		case 6:
			from_string(k, strTagData, dec);		// interval
			break;
		case 7:
			m_threshold = strTagData;
			break;
		case 8:
			m_yellow = strTagData;
			break;
		case 9:
			m_red = strTagData;
			break;
		/*case 11:
			uploadsite = strTagData;*/
		case 12:
			m_fromhost = strTagData;
			break;
		//case 13:
		//	m_spoolInterval = strTagData;
		//	break;
		}
	}
	/*FILETIME ft;
	ULARGE_INTEGER ui;
	GetSystemTimeAsFileTime(&ft);
	CopyMemory(&ui, &ft, sizeof(FILETIME));
	m_date = to_string(ui.QuadPart, dec);
	m_expires = to_string(ui.QuadPart + (j * 60), dec);
	m_interval = to_string(k * 60, dec);*/
	time_t t;
	time(&t);
	m_date = to_string(static_cast<long>(t), dec);
	m_expires = to_string(static_cast<long>(t) + (j * 60), dec);
	m_interval = to_string(k * 60, dec);
	//from_string(k, m_spoolInterval, dec);

	// Start the loop of monitoring
	while (1)
	{
		/*FILETIME ft;
		ULARGE_INTEGER ui;
		GetSystemTimeAsFileTime(&ft);
		CopyMemory(&ui, &ft, sizeof(FILETIME));
		m_date = to_string(ui.QuadPart, dec);*/
		time_t t;
		time(&t);
		m_date = to_string(static_cast<long>(t), dec);
		m_expires = to_string(static_cast<long>(t) + (j * 60), dec);
		// Determine if pausing or stopping is requested
		if (m_control == TRUE)
		{
			SetEvent(m_hEvent[0]);
			SuspendThread(GetCurrentThread());
		}
		else if (m_control == 2)
			break;
		// Monitor and output the result
		CMonitorOutput output;
		output.InitLogFile();
		output.SystemStat();
		output.ErrorLog();
		output.DiskFree();
		output.EndLogFile();
		WriteSpool(output.GetData());

		// Profile interval
		WaitForSingleObject(m_hEvent[0], k*1000);

		// Refresh current performance data
		sysInfo->RefreshInstances();
	}
Exit:
	sysInfo->Shutdown();
}

// This routine is to setup WMI queries
HRESULT	CMonitorOutput::SetupMonitor(CSystemInformation* pSysInfo)
{
	m_pSysInfo = pSysInfo;
	m_process = m_pSysInfo->AddPerfMon(CComBSTR(L"Win32_PerfRawData_PerfProc_Process"));
	m_memory = m_pSysInfo->AddPerfMon(CComBSTR(L"Win32_PerfRawData_PerfOS_Memory"));
	m_cpu_counter = m_pSysInfo->AddPerfMon(CComBSTR(L"Win32_PerfRawData_PerfOS_Processor"));
	m_disk = m_pSysInfo->AddQuery(CComBSTR(L"SELECT Name, FreeSpace, Size, FileSystem FROM Win32_LogicalDisk WHERE MediaType = 12"));
	m_cpu = m_pSysInfo->AddQuery(CComBSTR(L"SELECT LoadPercentage FROM Win32_Processor"));
	//m_fan = m_pSysInfo->AddQuery(CComBSTR(L"SELECT DesiredSpeed FROM Win32_Fan"));
	m_temp = m_pSysInfo->AddQuery(CComBSTR(L"SELECT NominalReading FROM Win32_TemperatureProbe"));
	return m_pSysInfo->RefreshInstances();
}

void CMonitorOutput::WriteSpool(string& strData)
{
	string strFile = SPOOL_DIRECTORY + string("\\") + m_date + "." + m_server + "." + m_fromhost + ".log";
	HANDLE hFile = CreateFile(strFile.c_str(), GENERIC_ALL, 0, NULL, CREATE_ALWAYS, 0, NULL);
	DWORD dwSize;
	WriteFile(hFile, strData.c_str(), static_cast<DWORD>(strData.size()), &dwSize, NULL);
	CloseHandle(hFile);
}

// Spool writing routine
void CMonitorOutput::InitLogFile()
{
	m_data << 
		"<?xml version=\"1.0\"?>\n" <<
		"<!DOCTYPE result SYSTEM \"/usr/share/upwatch/dtd/result.dtd\">\n" <<
		"<result xmlns=\"http://www.upwatch.com/schemas/1.0/\" fromhost=\"" << 
		m_fromhost << "\" date=\"" << m_date << "\">\n" << endl;
}

// The end of the log file
void CMonitorOutput::EndLogFile()
{
	m_data << "</result>" << endl;
}

// Get system stat data
void CMonitorOutput:: SystemStat()
{
	static ULONGLONG it[2], pt[2], ut[2], ts[2], pr[2], wr[2], ab[2], cb[2], mb[2], sb[2], mt[2];
	char* strTemp;
	int c[2] = { 0, 1 };
	vector<CComVariant> result;
	if (FAILED(m_pSysInfo->GetResult(1, m_cpu_counter, L"PercentProcessorTime", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(it[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_cpu_counter, L"PercentPrivilegedTime", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(pt[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_cpu_counter, L"PercentUserTime", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(ut[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_cpu_counter, L"Timestamp_Sys100NS", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(ts[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"PageReadsPerSec", result)))
		return;
	pr[c[0]] = result[0].intVal;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"PageWritesPerSec", result)))
		return;
	wr[c[0]] = result[0].intVal;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"AvailableBytes", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(ab[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"CommittedBytes", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(cb[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"CacheBytes", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(mb[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"SystemDriverResidentBytes", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(sb[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(1, m_memory, L"Timestamp_Sys100NS", result)))
		return;
	strTemp = _com_util::ConvertBSTRToString(result[0].bstrVal);
	from_string(mt[c[0]], strTemp, dec);
	delete[] strTemp;
	if (FAILED(m_pSysInfo->GetResult(0, m_cpu, L"LoadPercentage", result)))
		return;

	m_data << 
		"\t<sysstat realm=\"" << m_domain << "\" server=\"" << m_server << "\" ipaddress=\"" <<
		m_ipaddress << "\" date=\"" << m_date << "\" expires=\"" << m_expires << "\" color=\"" <<
		STAT_GREEN << "\" interval=\"" << m_interval << "\">\n";
	vector<CComVariant>::const_iterator iter;
	for (iter = result.begin();iter != result.end();iter++)
		m_data << "\t\t<loadavg>" << iter->intVal << "</loadavg>\n";
	m_data <<
		"\t\t<user>" << (int)((double)(ut[c[0]] - ut[c[1]])*100/(ts[c[0]] - ts[c[1]])) << "</user>\n" <<
		"\t\t<system>" << (int)((double)(pt[c[0]] - pt[c[1]])*100/(ts[c[0]] - ts[c[1]])) << "</system>\n" <<
		"\t\t<idle>" << (int)((double)(it[c[0]] - it[c[1]])*100/(ts[c[0]] - ts[c[1]])) << "</idle>\n" <<
        "\t\t<swapin>" << pr[c[0]] << "</swapin>\n" <<
        "\t\t<swapout>" << wr[c[0]] << "</swapout>\n" <<
		"\t\t<blockin>0</blockin>\n" <<
		"\t\t<blockout>0</blockout>\n" <<
		"\t\t<swapped>" << sb[c[0]] << "</swapped>\n" <<
        "\t\t<free>" << ab[c[0]] << "</free>\n" <<
		"\t\t<buffered>0</buffered>\n" <<
		"\t\t<cached>" << mb[c[0]] << "</cached>\n" <<
        "\t\t<used>" << cb[c[0]] << "</used>\n";
	HardwareInfo();
	ProcessList(result.begin()->intVal);
	m_data <<
		"\t</sysstat>\n";
	swap(c[0], c[1]);
}

// Get Hardware stat
void CMonitorOutput::HardwareInfo()
{
	vector<CComVariant> result;
	if (FAILED(m_pSysInfo->GetResult(0, m_temp, L"NominalReading", result)) || !result.size())
	{
		m_data << "\t\t<systemp>0</systemp>\n";
		return;
	}
    m_data << "\t\t<systemp>" << result[0].intVal << "</systemp>\n";
}

// Get Processlist, iLoad is the threshold passed to determine if result should be spooled or not
void CMonitorOutput::ProcessList(int iLoad)
{
	wchar_t* strName[] = { L"PercentPrivilegedTime", L"PercentUserTime", L"PercentProcessorTime", L"Timestamp_Sys100NS" };
	vector<CComVariant> pid, names, result;

	static map<int, vector<ULONGLONG> > process_time[2];
	static map<int, string> process_name[2];
	static vector<int> process_id[2];

	vector<CComVariant>::const_iterator iter[2];
	vector<int>::const_iterator pid_iter;

	if (FAILED(m_pSysInfo->GetResult(1, CMonitorOutput::m_process, L"IDProcess", pid)) ||
		FAILED(m_pSysInfo->GetResult(1, CMonitorOutput::m_process, L"Name", names)))
		return;
	static int i;
	process_time[i].clear();
	process_name[i].clear();
	process_id[i].clear();
	for (iter[0] = pid.begin(), iter[1] = names.begin();iter[0] != pid.end();iter[0]++, iter[1]++)
	{
		char* temp = _com_util::ConvertBSTRToString(iter[1]->bstrVal);
		process_id[i].push_back(iter[0]->intVal);
		process_name[i][iter[0]->intVal] = temp;
		delete[] temp;
	}
	for (int j = 0;j < 4;j++)
	{
		if (FAILED(m_pSysInfo->GetResult(1, CMonitorOutput::m_process, strName[j], result)))
			return;
		for (pid_iter = process_id[i].begin(), iter[0] = result.begin();pid_iter != process_id[i].end();iter[0]++, pid_iter++)
		{
			char* temp = _com_util::ConvertBSTRToString(iter[0]->bstrVal);
			ULONGLONG value;
			from_string(value, string(temp), dec);
			delete[] temp;
			process_time[i][*pid_iter].push_back(value);
		}
	}

	if (!i)
	{
		++i;
		return;
	}

	static int threshold;
	if (!threshold)
		from_string(threshold, m_threshold, dec);
	if (threshold < iLoad)
	{
        map<int, vector<double> > percentages;
		for (pid_iter = process_id[1].begin();pid_iter != process_id[1].end();pid_iter++)
		{
			if (find(process_id[0].begin(), process_id[0].end(), *pid_iter) == process_id[0].end())
				continue;
			percentages[*pid_iter].resize(8);
			transform(process_time[1][*pid_iter].begin(), process_time[1][*pid_iter].end(),
				process_time[0][*pid_iter].begin(), process_time[0][*pid_iter].begin(), minus<ULONGLONG>());
			transform(process_time[0][*pid_iter].begin(), process_time[0][*pid_iter].end(),
				percentages[*pid_iter].begin(), bind2nd(divides<double>(), process_time[0][*pid_iter][3]));
		}

		m_data << "\t\t<info>\n";
		m_data << "\t\t\t" << right << setw(6) << "PID" << " " << setw(20) << "Process" << " " <<
			left << setw(10) << "Priv CPU%" << " " << setw(10) << "User CPU%" << " " << "Processor CPU%\n";
		for (pid_iter = process_id[1].begin();pid_iter != process_id[1].end();pid_iter++)
		{
			if (find(process_id[0].begin(), process_id[0].end(), *pid_iter) == process_id[0].end() || *pid_iter == 0)
				continue;
			m_data << "\t\t\t" << right << setw(6) << *pid_iter << " " << setw(20) << process_name[0][*pid_iter] << " ";
			vector<double>::const_iterator percentage_iter = percentages[*pid_iter].begin();
			for (;percentage_iter != percentages[*pid_iter].begin() + 3;percentage_iter++)
			{
				int value = static_cast<int>(*percentage_iter * 100);
				m_data << setw(10) << value << " ";
			}
			m_data << "\n";
		}
		m_data << "\t\t</info>\n";
	}

	process_time[0] = process_time[1];
	process_name[0] = process_name[1];
	process_id[0] = process_id[1];
}

// Get the event log result and spool it into the event log directory
void CMonitorOutput::ErrorLog()
{
	HANDLE hFile = CreateFile(SPEC_FILE, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	vector<_errlogspec> specs;
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = GetFileSize(hFile, NULL);
		char* buffer = new char[dwSize + 1];
		ReadFile(hFile, buffer, dwSize, &dwSize, NULL);
		string strBuffer(buffer);
		delete[] buffer;

		string::size_type offset = 0, pos = strBuffer.find('\n');
		while (pos != string::npos)
		{
			_errlogspec s = {0};
			string path = strBuffer.substr(offset, pos);
			string::size_type trim = path.find('\r');
			if (trim != string::npos)
				path.replace(trim, 1, "");
			
			s.path = new char[path.size() + 1];
			copy(path.begin(), path.end(), s.path);
			s.path[path.size()] = 0;

			string style = "abc";
			s.style = new char[style.size() + 1];
			copy(style.begin(), style.end(), s.style);
			s.style[style.size()] = 0;
			specs.push_back(s);

			offset = pos + 1;
			pos = strBuffer.find('\n', offset);
		}
		CloseHandle(hFile);
	}
	_errlogspec s = {0};
	specs.push_back(s);
	errlogspec = &specs[0];

	HANDLE h;
    EVENTLOGRECORD *pevlr; 
    static BYTE bBuffer[0x5000];
    DWORD dwRead, dwNeeded, dwThisRecord, dwNumRecords;

	FILETIME FileTime, LocalFileTime;
	SYSTEMTIME SysTime;
	LONGLONG lgTemp;
	static LONGLONG SecsTo1970 = 116444736000000000;

	string eventlog[] = { "Application", "Security", "System" };
	static DWORD dwLatestRecord[3] = {0};
	if (dwLatestRecord[0] == 0)
	{
		hFile = CreateFile(RECORD_FILE, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			ReadFile(hFile, dwLatestRecord, 3*sizeof(DWORD), &dwRead, NULL);
			CloseHandle(hFile);
		}
	}
 
	// Enumerate the event log with records and output them into text file
	for (int i = 0;i < 3;i++)
	{
        h = OpenEventLog(NULL, eventlog[i].c_str());
		if (h == NULL) 
			continue;
        pevlr = reinterpret_cast<PEVENTLOGRECORD>(&bBuffer);
	
		static DWORD dwOldRecord[3], dwNumberOfRecords[3];
		GetOldestEventLogRecord(h, &dwThisRecord);
		GetNumberOfEventLogRecords(h, &dwNumRecords);
		if (dwOldRecord[i] == dwThisRecord && dwNumberOfRecords[i] == dwNumRecords)
		{
			CloseEventLog(h);
			continue;
		}
		dwOldRecord[i] = dwThisRecord;
		dwNumberOfRecords[i] = dwNumRecords;

        vector<string> logs;
		while (ReadEventLog(h, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 0,
			pevlr, 0x5000,  &dwRead, &dwNeeded))
		{
			while (dwRead > 0) 
			{ 
				if (pevlr->TimeGenerated <= dwLatestRecord[i])
				{
					dwRead -= pevlr->Length; 
					pevlr = reinterpret_cast<EVENTLOGRECORD*>(reinterpret_cast<PBYTE>(pevlr) + pevlr->Length);
					continue;
				}
				else
					dwLatestRecord[i] = pevlr->TimeGenerated;
                strstream strLog;
				lgTemp = Int32x32To64(pevlr->TimeGenerated,10000000) + SecsTo1970;
		
				FileTime.dwLowDateTime = static_cast<DWORD>(lgTemp);
				FileTime.dwHighDateTime = static_cast<DWORD>(lgTemp >> 32);

                FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
                FileTimeToSystemTime(&LocalFileTime, &SysTime);

				strLog << setw(2) << SysTime.wMonth << "/" << SysTime.wDay << "/" <<  SysTime.wYear << " "
					<< SysTime.wHour << ":" << SysTime.wMinute << ":" << SysTime.wSecond << "\t";
				
				strLog << setw(4) << dwThisRecord++ << " ID: " << hex <<
					setw(8) << pevlr->EventID << " ";
				
				switch(pevlr->EventType)
				{
                case EVENTLOG_ERROR_TYPE:
                    strLog << "EVENTLOG_ERROR_TYPE\t\t";
                    break;
                case EVENTLOG_WARNING_TYPE:
                    strLog << "EVENTLOG_WARNING_TYPE\t\t";
                    break;
                case EVENTLOG_INFORMATION_TYPE:
                    strLog << "EVENTLOG_INFORMATION_TYPE\t";
                    break;
                case EVENTLOG_AUDIT_SUCCESS:
                    strLog << "EVENTLOG_AUDIT_SUCCESS\t";
                    break;
                case EVENTLOG_AUDIT_FAILURE:
                    strLog << "EVENTLOG_AUDIT_FAILURE\t";
                    break;
                default:
                    strLog << "Unknown Event Type\t";
                    break;
				}
				
				char* source = reinterpret_cast<char*>(&pevlr[1]);
				strLog << "Source: " << setw(20) << left << source;
				
				string key = "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\" + eventlog[i] + "\\" + string(source);
				HKEY hKey;
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
					goto Next;
				
				char sz[0xff*0xff];
				DWORD dw = sizeof(sz);
				dw = RegQueryValueEx(hKey, "EventMessageFile", NULL, NULL, reinterpret_cast<PBYTE>(sz), &dw);
				RegCloseKey(hKey);
				if (dw != ERROR_SUCCESS)
					goto Next;
			
				ExpandEnvironmentStrings(string(sz).c_str(), sz, sizeof(sz));
				string::size_type pos = string(sz).find(";");
				if (pos == string::npos)
					pos = string(sz).size();
				HMODULE hModule = LoadLibrary(string(sz).substr(0, pos).c_str());
				if (!hModule)
					goto Next;

				PVOID* Args = new PVOID[pevlr->NumStrings + 1];
				size_t offset = pevlr->StringOffset;
				for (int j = 0;j < pevlr->NumStrings;j++)
				{
					Args[j] = reinterpret_cast<PBYTE>(pevlr) + offset;
					offset += string(static_cast<char*>(Args[j])).size() + 1;
				}
				Args[j] = 0;
				FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					hModule, pevlr->EventID, 0, sz, sizeof(sz), reinterpret_cast<va_list*>(Args));
				delete[] Args;
				FreeLibrary(hModule);
				strLog << "Message: " << sz << "\n";
Next:
				strLog << '\0';
				dwRead -= pevlr->Length; 
				pevlr = reinterpret_cast<EVENTLOGRECORD*>(reinterpret_cast<PBYTE>(pevlr) + pevlr->Length);

				logs.push_back(strLog.str());
			}
			pevlr = reinterpret_cast<PEVENTLOGRECORD>(&bBuffer);
		}
		CloseEventLog(h);

		string filename = LOG_DIRECTORY + string("\\") + eventlog[i] + "\\logfile";
		ofstream output(filename.c_str());
		vector<string>::const_iterator iter = logs.begin();
		for (;iter != logs.end();iter++)
			output << *iter << "\n";
		output.close();
	}
	hFile = CreateFile(RECORD_FILE, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, dwLatestRecord, 3*sizeof(DWORD), &dwRead, NULL);
		CloseHandle(hFile);
	}
	
	// Here analyse the event log and output the required color
	static int highest_color = STAT_GREEN;
	int color = highest_color;
	GString *log = check_logs(&color);
	if (color > highest_color)
		highest_color = color;

	m_data << 
		"\t<errlog realm=\"" << m_domain << "\" server=\"" << m_server << "\" ipaddress=\"" <<
		m_ipaddress << "\" date=\"" << m_date << "\" expires=\"" << m_expires << "\" color=\"" << 
		color << "\" interval=\"" << m_interval << "\">\n";
	if (log)
	{
		if (log->str && strlen(log->str) > 0)
			m_data << "\t\t<info>" << log->str << "\n\t\t</info>\n";
		g_string_free(log, TRUE);
	}
	m_data << "\t</errlog>\n";

	vector<_errlogspec>::const_iterator spec_iter = specs.begin();
	for (;spec_iter != specs.end();spec_iter++)
	{
		delete[] spec_iter->style;
		delete[] spec_iter->path;
	}
	errlogspec = NULL;
}

// Get the disk information
void CMonitorOutput::DiskFree()
{
	vector<vector<CComVariant> >diskinfo;
	vector<CComVariant> result;
	vector<ULONGLONG> freespace, percentage;
	if (FAILED(m_pSysInfo->GetResult(0, m_disk, L"Name", result)))
		return;
	
	diskinfo.push_back(result);
	if (FAILED(m_pSysInfo->GetResult(0, m_disk, L"FreeSpace", result)))
		return;
	diskinfo.push_back(result);
	
	freespace.resize(result.size());
	transform(result.begin(), result.end(), freespace.begin(), &from_string_1<ULONGLONG>);
	transform(freespace.begin(), freespace.end(), freespace.begin(), bind2nd(multiplies<ULONGLONG>(), 100));
	
	if (FAILED(m_pSysInfo->GetResult(0, m_disk, L"Size", result)))
		return;
	diskinfo.push_back(result);
	
	percentage.resize(result.size());
	transform(result.begin(), result.end(), percentage.begin(), &from_string_1<ULONGLONG>);
	transform(freespace.begin(), freespace.end(), percentage.begin(), percentage.begin(), divides<ULONGLONG>());
	
	if (FAILED(m_pSysInfo->GetResult(0, m_disk, L"FileSystem", result)))
		return;
	diskinfo.push_back(result);

	SHORT color = STAT_GREEN, yellow, red, max_percentage = 100 - static_cast<SHORT>(*min_element(percentage.begin(), percentage.end()));
	from_string(yellow, m_yellow, dec);
	from_string(red, m_red, dec);
	if (max_percentage > yellow && max_percentage < red)
		color = STAT_YELLOW;
	else if (max_percentage > red)
		color = STAT_RED;

	m_data << 
		"\t<diskfree realm=\"" << m_domain << "\" server=\"" << m_server << "\" ipaddress=\"" <<
		m_ipaddress << "\" date=\"" << m_date << "\" expires=\"" << m_expires << "\" color=\"" << 
		color << "\" interval=\"" << m_interval << "\">\n";
	m_data << "\t\t<info>\n\t\t" << 
		setw(22) << "Filesystem" <<
		setw(22) << "K-blocks" <<
		setw(22) << "Used" <<
		setw(22) << "Avail" << " " <<
		setw(10) << "Capacity" << " " << "Mounted\n";
	for (unsigned i = 0;i < result.size();i++)
	{
		ULONGLONG freespace, size, capacity;
		char* strTemp = _com_util::ConvertBSTRToString(diskinfo[1][i].bstrVal);
		from_string(freespace, strTemp, dec);
		delete[] strTemp;

		strTemp = _com_util::ConvertBSTRToString(diskinfo[2][i].bstrVal);
		from_string(size, strTemp, dec);
		delete[] strTemp;

		strTemp = _com_util::ConvertBSTRToString(diskinfo[0][i].bstrVal);

		char* strFileSystem = _com_util::ConvertBSTRToString(diskinfo[3][i].bstrVal);
		capacity = size - freespace;
		m_data << "\t\t" <<
			setw(22) << strFileSystem <<
			setw(22) << size <<
			setw(22) << capacity <<
			setw(22) << freespace << " " <<
			setw(10) << 100*((double)capacity)/size << " "
			<< strTemp << "\n";
		delete[] strFileSystem;
		delete[] strTemp;
	}
	m_data << "\t\t</info>\n\t</diskfree>\n";
}

// Load the data from configuration file
string CMonitorOutput::LoadData()
{
	HANDLE hFile = CreateFile(CONFIG_FILE, GENERIC_READ, 0, NULL, OPEN_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return string("");
	DWORD size = GetFileSize(hFile, NULL);
	LPTSTR data = new TCHAR[size];
	ReadFile(hFile, data, size, &size, NULL);
	CloseHandle(hFile);

	return data;
}

string CMonitorOutput::SetTagData(const string& strTag, const string& szTagData)
{
	return "<" + strTag + ">" + szTagData + "</" + strTag + ">";
}

string CMonitorOutput::GetTagData(const string& szData, const string& strTag)
{
	string::size_type pos = szData.find("<" + strTag);
	if (pos == -1)
		return string("");
	pos = szData.find(">", pos) + 1;
	string data = szData.substr(pos, szData.find("</" + strTag + ">") - pos);
	return data;
}