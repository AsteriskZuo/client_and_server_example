#include "tcp_socket.h"



tcp_socket::tcp_socket()
{
	static int sid = 0;
	id_ = sid++;
	addrinfo_ = NULL;
	socket_ = 0;
	is_server_ = false;
}


tcp_socket::~tcp_socket()
{
	close_socket();
}

bool tcp_socket::init()
{
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		//printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
	return true;
}

void tcp_socket::uninit()
{
	WSACleanup();
}

bool tcp_socket::create_socket(bool is_server /*= false*/)
{
	if (0 != socket_)
		return true;

	is_server_ = is_server;

	struct addrinfo hints;
	int iResult = 0;

	ZeroMemory(&hints, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (is_server_) {
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;
	}
	else {
		hints.ai_family = AF_UNSPEC;
	}
	socket_ = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (socket_ == INVALID_SOCKET) {
		//printf("socket failed with error: %ld\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool tcp_socket::connect_to_server(const std::string& host, const std::string& port)
{
	host_ = host;
	port_ = port;

	if (!get_addr_info())
		return false;

	int iResult = 0;
	iResult = connect(socket_, addrinfo_->ai_addr, (int)addrinfo_->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		return false;
	}
	return true;
}

bool tcp_socket::bind_socket(const std::string& host, const std::string& port)
{
	host_ = host;
	port_ = port;

	if (!get_addr_info())
		return false;

	int iResult = 0;
	iResult = bind(socket_, addrinfo_->ai_addr, (int)addrinfo_->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		//printf("bind failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool tcp_socket::listen_socket()
{
	int iResult = 0;
	iResult = listen(socket_, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		//printf("listen failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool tcp_socket::accept_socket()
{
	SOCKET client_socket = accept(socket_, NULL, NULL);
	if (client_socket == INVALID_SOCKET) {
		//printf("accept failed with error: %d\n", WSAGetLastError());
		return false;
	}
	client_sockets_.push_back(client_socket);
	return true;
}

bool tcp_socket::shutdown_socket()
{
	int iResult = 0;
	iResult = shutdown(socket_, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		//printf("shutdown failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void tcp_socket::close_socket()
{
	free_addr_info();
	closesocket(socket_);
	socket_ = 0;
}

bool tcp_socket::disconnect_all_client_socket()
{
	for (auto iter= client_sockets_.begin(); iter != client_sockets_.end(); ++iter)
	{
		shutdown(*iter, SD_SEND);
		closesocket(*iter);
	}
	return true;
}

bool tcp_socket::send_data(const char* data, int data_size)
{
	int iResult = 0;
	iResult = send(socket_, data, data_size, 0);
	if (iResult == SOCKET_ERROR) {
		//printf("send failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool tcp_socket::recv_data(char* data, int data_size, int& really_size)
{
	int iResult = 0;
	iResult = recv(socket_, data, data_size, 0);//maybe block
	if (iResult == SOCKET_ERROR) {
		//printf("recv failed with error: %d\n", WSAGetLastError());
		return false;
	}
	really_size = iResult;
	return true;
}

bool tcp_socket::get_addr_info()
{
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	int iResult = 0;

	ZeroMemory(&hints, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (is_server_) {
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;
	}
	else {
		hints.ai_family = AF_UNSPEC;
	}
	iResult = getaddrinfo(host_.c_str(), port_.c_str(), &hints, &result);
	if (iResult != 0) {
		//printf("getaddrinfo failed with error: %d\n", iResult);
		return false;
	}
	addrinfo_ = result;

	return true;
}

void tcp_socket::free_addr_info()
{
	if (addrinfo_)
		freeaddrinfo(addrinfo_);
	addrinfo_ = NULL;
}
