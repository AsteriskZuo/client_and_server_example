#include "tcp_connection.h"
#include "tcp_socket.h"
#include <sstream>
#include <iostream>
#include <cassert>

#define DEFAULT_SIZE 1024



tcp_packet_original::tcp_packet_original()
	:data(nullptr), head(0), checknum(0), calchecknum(0), bodysize(0), datapos(0), charbodysizelen(0), charbodysize{ 0 }
{

}
tcp_packet_original::tcp_packet_original(tcp_packet_original& o)
{
	this->swap(*this, o);
}
tcp_packet_original& tcp_packet_original::operator=(tcp_packet_original& o)
{
	if (this != &o)
		this->swap(*this, o);
	return *this;
}
void tcp_packet_original::reset()
{
	if (data) delete data; data = nullptr;
	head = 0;
	checknum = 0;
	calchecknum = 0;
	bodysize = 0;
	datapos = 0;
	std::memset(charbodysize, 0, 4);
	charbodysizelen = 0;
}

bool tcp_packet_original::is_done()
{
	return (bodysize + 6 == datapos && datapos != 0) ? true : false;
}

void tcp_packet_original::swap(tcp_packet_original& l, tcp_packet_original& r)
{
	if (&l != &r)
	{
		std::swap(l.data, r.data);
		std::swap(l.head, r.head);
		std::swap(l.checknum, r.checknum);
		std::swap(l.calchecknum, r.calchecknum);
		std::swap(l.bodysize, r.bodysize);
		std::swap(l.datapos, r.datapos);
		std::swap(l.charbodysize, r.charbodysize);
		std::swap(l.charbodysizelen, r.charbodysizelen);
	}
}


tcp_connection::tcp_connection()
	: send_msg_worker_(nullptr)
	, recv_msg_worker_(nullptr)
	, timeout_monitor_(nullptr)
{
	is_init_.store(false);
	connect_status_.store(0);
	worker_status_.store(false);
	user_quit_app_.store(false);
}

tcp_connection::~tcp_connection()
{
}

void tcp_connection::init()
{
	/*
		* only once
		* Must be the first call.
	*/
	is_init_.store(true);
	tcp_socket::init();
	start();
}

void tcp_connection::uninit()
{
	/*
		* only once
		* Must be the last call.
	*/
	is_init_.store(false);
	tcp_socket::uninit();
	stop();
}

void tcp_connection::connect(const tcp_info& info)
{
	/*
		* Connect and disconnect one by one correspond.
		* Connect must be preceded by a disconnect.
		* Connect is block function.
	*/
	_connect(info);
}

void tcp_connection::disconnect()
{
	/*
		* Disconnect is block function.
		* wait recv msg end.
	*/
	connect_status_.store(0);
	_disconnect();
}

void tcp_connection::quit()
{
	/*
		* End worker threads
	*/
	user_quit_app_.store(true);
	send_msg_tasks_.quit();
	recv_msg_tasks_.quit();
}

void tcp_connection::send_msg(const send_msg_info& info)
{
	send_msg_tasks_.push_task(std::make_shared<send_msg_info>(info));
}

void tcp_connection::recv_msg(const recv_msg_info& info)
{
	recv_msg_tasks_.push_task(std::make_shared<recv_msg_info>(info));
}

void tcp_connection::start()
{
	workers_result_ = std::async(std::launch::async, &tcp_connection::start_internal, shared_from_this());
}

void tcp_connection::stop()
{
	workers_result_.get();//wait threads stop
}

void tcp_connection::start_internal()
{
	worker_status_.store(true);
	send_msg_worker_ = new std::thread(&tcp_connection::run_for_send_msg, shared_from_this());
	recv_msg_worker_ = new std::thread(&tcp_connection::run_for_recv_msg, shared_from_this());
	timeout_monitor_ = new std::thread(&tcp_connection::run_for_control, shared_from_this());
	if (send_msg_worker_->joinable())
		send_msg_worker_->join();
	if (recv_msg_worker_->joinable())
		recv_msg_worker_->join();
	if (timeout_monitor_->joinable())
		timeout_monitor_->join();
	delete send_msg_worker_; send_msg_worker_ = nullptr;
	delete recv_msg_worker_; recv_msg_worker_ = nullptr;
	delete timeout_monitor_; timeout_monitor_ = nullptr;
	worker_status_.store(false);
}

void tcp_connection::run_for_send_msg()
{
	for (;;)
	{
		if (user_quit_app_.load())
			break;
		//auto task = send_msg_tasks_.obtain_task();
		const std::shared_ptr<send_msg_info> task = send_msg_tasks_.obtain_task();
		if (socket_)
			socket_->send_data(task->data, task->datasize);//暂时发送小于65535字节的消息
		if (task.get())
			(*task)();
	}
}

void tcp_connection::run_for_recv_msg()
{
	for (;;)
	{
		if (user_quit_app_.load())
			break;
		//auto task = recv_msg_tasks_.obtain_task();
		const std::shared_ptr<recv_msg_info> task = recv_msg_tasks_.obtain_task();
		if (task.get())
			(*task)();
	}
}

void tcp_connection::run_for_control()
{
	for (;;)
	{
		if (user_quit_app_.load())
			break;
	}
}

void tcp_connection::start_recv()
{
	recv_result_ = std::async(std::launch::async, &tcp_connection::start_recv_internal, shared_from_this());
}

void tcp_connection::stop_recv()
{
	recv_result_.get();
}

void tcp_connection::start_recv_internal()
{
	recv_msg_from_server = new std::thread(&tcp_connection::recv_msg_from_server, shared_from_this());
	if (recv_msg_from_server->joinable())
		recv_msg_from_server->join();
	delete recv_msg_from_server; recv_msg_from_server = nullptr;
}

void tcp_connection::run_for_server()
{
	/*
		* 情况：
			* 1.单个协议大于最大接收阈值 [粘包]
			* 2.多个协议一次接收 [拆包]
			* 3.多个协议一次接收，部分协议接收一部分 [拆包][粘包]
			* 4.理想状态：单个协议一次接收完成，并且没有其它协议
		* 拆包
		* 粘包
	*/

	char recv_data[DEFAULT_SIZE];
	std::memset(recv_data, 0, DEFAULT_SIZE);
	tcp_packet_original packet;
	int step = 0;//0.init 1.head is_done 2.checknum is_done 3.bodysize is_done 4.body is_done

	for (;;)
	{
		if (user_quit_app_.load())
			break;
		int really_size = 0;
		int recv_data_pos = 0;
		if (socket_ && socket_->recv_data(recv_data, DEFAULT_SIZE, really_size)) 
		{
			recv_data_pos = 0;
			while (0 < really_size - recv_data_pos)
			{
				if (0 == packet.datapos)
				{
					packet.head = *(recv_data + recv_data_pos);
					++packet.datapos;
					++recv_data_pos;
				}
				if (0 == really_size - recv_data_pos)
				{
					std::string originalhex = string_to_hex(std::string(recv_data, really_size));
					std::cout << "original hex: " << originalhex << std::endl;
					break;
				}

				if (1 == packet.datapos)
				{
					packet.checknum = *(recv_data + recv_data_pos);
					++packet.datapos;
					++recv_data_pos;
				}
				if (0 == really_size - recv_data_pos)
				{
					std::string originalhex = string_to_hex(std::string(recv_data, really_size));
					std::cout << "original hex: " << originalhex << std::endl;
					break;
				}

				if (2 <= packet.datapos)
				{
					if (0 == packet.charbodysizelen)
					{
						//calculate body size len
						uint8_t tmp = packet.datapos - 2;
						while (4 > packet.charbodysizelen && 0 < really_size - recv_data_pos)
						{
							if (is_high_pos_byte(*(recv_data + recv_data_pos)))
							{
								packet.charbodysize[tmp] = *(recv_data + recv_data_pos);
								packet.charbodysizelen = ++tmp;
								assert(4 > packet.charbodysizelen);
								++packet.datapos;
								++recv_data_pos;
								get_body_size(packet.charbodysize, packet.charbodysizelen, packet.bodysize, packet.calchecknum);
								break;
							}
							packet.charbodysize[tmp] = *(recv_data + recv_data_pos);
							++tmp;
							++packet.datapos;
							++recv_data_pos;
						}
						if (0 == really_size - recv_data_pos)
						{
							std::string originalhex = string_to_hex(std::string(recv_data, really_size));
							std::cout << "original hex: " << originalhex << std::endl;
							break;
						}
					}
					else
					{
						if (2 + packet.charbodysizelen == packet.datapos)
						{
							packet.data = new char[packet.bodysize + 2 + packet.charbodysizelen];
							std::memcpy(packet.data, &packet.head, 1);
							std::memcpy(packet.data + 1, &packet.checknum, 1);
							std::memcpy(packet.data + 2, &packet.charbodysize, packet.charbodysizelen);
						}
						for (int i = packet.datapos;
							2 + packet.charbodysizelen <= packet.datapos
							&& packet.bodysize > packet.datapos - 2 - packet.charbodysizelen
							&& really_size - recv_data_pos > 0; ++i)
						{
							packet.data[i] = *(recv_data + recv_data_pos);
							++packet.datapos;
							++recv_data_pos;
						}
						if (packet.bodysize + 2 + packet.charbodysizelen == packet.datapos)
						{
							//is_done

							std::string originalhex = string_to_hex(std::string(recv_data, really_size));
							std::string changedhex = string_to_hex(std::string(packet.data, packet.datapos));
							std::string testdatabody = std::string(packet.data + 2 + packet.charbodysizelen, packet.bodysize);


							std::cout << "original hex: " << originalhex << std::endl;
							std::cout << "changed hex: " << changedhex << std::endl
								<< " data: " << testdatabody << std::endl
								<< std::endl;


							tcp_packet_original recv_packet;
							recv_packet = packet;
							//packet.reset();

						}
						if (0 == really_size - recv_data_pos)
						{
							std::string originalhex = string_to_hex(std::string(recv_data, really_size));
							std::cout << "original hex: " << originalhex << std::endl;
							break;
						}
					}
				}

			}

		}
	}
}

std::string tcp_connection::string_to_hex(const std::string& str)
{
	const std::string hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (std::string::size_type i = 0; i < str.size(); ++i)
		ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf];
	return ss.str();
}

std::string tcp_connection::hex_to_string(const std::string& hex)
{
	std::string result;
	for (size_t i = 0; i < hex.length(); i += 2)
	{
		std::string byte = hex.substr(i, 2);
		char chr = (char)(int)std::strtol(byte.c_str(), NULL, 16);
		result.push_back(chr);
	}
	return result;
}

bool tcp_connection::_connect(const tcp_info& info)
{
	if (!socket_)
		socket_ = std::make_shared<tcp_socket>();
	if (!socket_->create_socket()) {
		socket_->close_socket();
		return false;
	}
	if (!socket_->connect_to_server(info.host, info.port)) {
		return false;
	}
	start_recv();
	return true;
}

void tcp_connection::_disconnect()
{
	if (socket_)
		socket_->close_socket();
	stop_recv();
	socket_.reset();
}

bool tcp_connection::is_high_pos_byte(const char data)
{
	uint8_t d = data;
	if (d & 0x80)
		return false;
	else
		return true;
}

bool tcp_connection::get_body_size(const char* cbodysize, const uint8_t cbodysizelen, uint32_t& bodysize, char& checkcode)
{
	bool ret = false;
	if (cbodysize)
	{
		uint32_t multi = 1;
		for (int i = 0; i < cbodysizelen; ++i)
		{
			uint8_t d = cbodysize[i];
			bodysize += (d & 0x7f) * multi;
			multi *= 0x80;
			checkcode ^= cbodysize[i];
		}
		return true;
	}
	return ret;
}
bool tcp_connection::get_char_body_size(const uint32_t bodysize, char* cbodysize, uint8_t& cbodysizelen)
{
	bool ret = false;
	if (cbodysize)
	{
		cbodysizelen = 0;
		uint32_t tmp = bodysize;
		while (0 < tmp)
		{
			uint8_t d = tmp % 0x80;
			tmp = tmp / 0x80;
			if (0 < tmp)
				d |= 0x80;
			cbodysize[cbodysizelen++] = d;
		}
		return true;
	}
	return ret;
}

void send_msg_info::operator()()
{

}

void recv_msg_info::operator()()
{

}

void tcp_packet_info::operator()()
{

}
