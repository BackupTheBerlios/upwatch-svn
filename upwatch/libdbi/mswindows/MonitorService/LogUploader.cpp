#include "main.h"
#include ".\loguploader.h"

using namespace std;

CLogUploader* CLogUploader::m_pLogUploader = NULL;

CLogUploader* CLogUploader::GetUploader()
{
	if (CLogUploader::m_pLogUploader == NULL)
		CLogUploader::m_pLogUploader = new CLogUploader();
	return CLogUploader::m_pLogUploader;
}

void CLogUploader::ShutdownUploader()
{
	delete CLogUploader::m_pLogUploader;
	CLogUploader::m_pLogUploader = NULL;
}

CLogUploader::CLogUploader()
:m_socket_is_busy(false)
{
	WSAStartup(MAKEWORD(2, 2), &m_wsaData);
}

CLogUploader::~CLogUploader()
{
	WSACleanup();
}

void CLogUploader::SetupUploaderInfo(std::string& username, 
									 std::string& password, 
									 std::string& ip_address, 
									 u_short port)
{
	m_username = "user " + username + "\n";
	m_password = "pass " + password + "\n";
	m_ok = "+OK";
	m_quit = "quit\n";

	m_client.sin_family = AF_INET;
	m_client.sin_addr.s_addr = inet_addr(ip_address.c_str());
	m_client.sin_port = htons(port);
}

int CLogUploader::Login()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int result = connect(m_socket, reinterpret_cast<sockaddr*>(&m_client), sizeof(m_client));
	if (result == SOCKET_ERROR)
		result = WSAGetLastError();
	else
	{
		m_socket_is_busy = true;
		char buffer[0x32];
		string::size_type size = m_ok.size();
		recv(m_socket, buffer, sizeof(buffer), 0);
		if (string(buffer).substr(0, size) == m_ok)
			send(m_socket, m_username.c_str(), static_cast<int>(m_username.size()), 0);
		recv(m_socket, buffer, sizeof(buffer), 0);
		if (string(buffer).substr(0, size) == m_ok)
			send(m_socket, m_password.c_str(), static_cast<int>(m_password.size()), 0);
		recv(m_socket, buffer, sizeof(buffer), 0);
		if (string(buffer).substr(0, size) != m_ok)
			result = -1;
	}
	return result;
}

int CLogUploader::Command(std::string& command)
{
	return Command(command.c_str(), static_cast<DWORD>(command.size()));
}

int CLogUploader::Command(const char* pData, DWORD dwSize)
{
	if (send(m_socket, pData, dwSize, 0) == SOCKET_ERROR)
		return WSAGetLastError();
	char buffer[0x32];
	recv(m_socket, buffer, sizeof(buffer), 0);
	if (string(buffer).substr(0, m_ok.size()) == m_ok)
		return 0;
	else
		return -1;
}

int CLogUploader::Quit()
{
	send(m_socket, m_quit.c_str(), static_cast<int>(m_quit.size()), 0);
	m_socket_is_busy = false;
	return closesocket(m_socket);
}