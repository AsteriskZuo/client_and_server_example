#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <list>
#include <memory>

#pragma comment (lib, "Ws2_32.lib")


class tcp_socket : public std::enable_shared_from_this<tcp_socket>
{
public:
	enum SocketStatus
	{
		init_status = 0,
		connected_status,
		disconnected_status,
		connecting_status,
	};

	tcp_socket();
	~tcp_socket();

	static bool init();
	static void uninit();

	bool create_socket(bool is_server = false);
	void close_socket();
	bool connect_to_server(const std::string& host, const std::string& port);
	bool bind_socket(const std::string& host, const std::string& port);
	bool listen_socket();
	bool accept_socket();
	bool shutdown_socket();


	bool disconnect_all_client_socket();

	bool send_data(const char* data, int data_size);
	bool recv_data(char* data, int data_size, int& really_size);//callback

	int socket_id() { return id_; }
	bool is_server() { return is_server_; }

private:
	bool get_addr_info();
	void free_addr_info();

private:
	int id_;
	SOCKET socket_;
	bool is_server_;
	std::string host_;
	std::string port_;
	struct addrinfo *addrinfo_;
	std::list<SOCKET> client_sockets_;//if server
};

