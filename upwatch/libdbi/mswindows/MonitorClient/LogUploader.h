#pragma once

#pragma comment(lib, "WS2_32.lib")

#include <string>

class CLogUploader
{
public:
	static CLogUploader* GetUploader();
	static void ShutdownUploader();
	void SetupUploaderInfo(std::string& username, std::string& password, std::string& ip_address, u_short port);

	int Login();
	int Command(std::string& command);
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
