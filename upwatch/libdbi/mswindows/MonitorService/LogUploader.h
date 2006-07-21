#pragma once

#pragma comment(lib, "WS2_32.lib")

#include <string>

class CLogUploader
{
public:
	// Initializing the uploader
	static CLogUploader* GetUploader();
	// Shut down the uploader
	static void ShutdownUploader();
	// Setup the uploader info (e.g username/password ipaddress)
	void SetupUploaderInfo(std::string& username, std::string& password, std::string& ip_address, u_short port);

	// The loging procedure
	int Login();
	// Execute upwatch command
	int Command(std::string&);
	int Command(const char*, DWORD);
	// Quit upwatch connection
	int Quit();
	bool IsSocketBusy() { return m_socket_is_busy; }
private:
	CLogUploader();
	~CLogUploader();

	static CLogUploader*	m_pLogUploader;
	bool		m_socket_is_busy;

	WSAData		m_wsaData;
	SOCKET		m_socket;
	sockaddr_in	m_client;

	std::string	m_username;
	std::string	m_password;
	std::string	m_ok;
	std::string m_quit;
};
