// test_async.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cassert>

void work1()
{
	std::cout << __FUNCTION__ << ":in :tid " << std::this_thread::get_id() << std::endl;
	std::chrono::seconds s(5);
	std::chrono::duration<long long, std::micro> ms;
	std::chrono::duration<long long, std::ratio<1, 1000000>> ms2;
	std::this_thread::sleep_for(s);
	std::cout << __FUNCTION__ << ":out :tid " << std::this_thread::get_id() << std::endl;
}

std::future<void> sfuture;
void test_async()
{
	std::cout << __FUNCTION__ << ":in :tid " << std::this_thread::get_id() << std::endl;
	sfuture = std::async(std::launch::async, work1);
	std::cout << __FUNCTION__ << ":out :tid " << std::this_thread::get_id() << std::endl;
}
void test_async_result()
{
	std::cout << __FUNCTION__ << ":in :tid " << std::this_thread::get_id() << std::endl;
	sfuture.get();
	std::cout << __FUNCTION__ << ":out :tid " << std::this_thread::get_id() << std::endl;
}

void test_swap()
{
	const char* p = "test1";
	const char* p2 = "test2";
	std::cout << "p:" << p << " " << std::hex << (int)&p << std::endl;
	std::cout << "p2:" << p2 << " " << std::hex << (int)&p2 << std::endl;
	std::swap(p, p2);
	std::cout << "p:" << p << " " << std::hex << (int)&p << std::endl;
	std::cout << "p2:" << p2 << " " << std::hex << (int)&p2 << std::endl;
}

void test_swap2()
{
	char p[] = "test1";
	char p2[] = "test2";
	std::cout << "p:" << p << " " << std::hex << (int)&p << std::endl;
	std::cout << "p2:" << p2 << " " << std::hex << (int)&p2 << std::endl;
	std::swap(p, p2);
	std::cout << "p:" << p << " " << std::hex << (int)&p << std::endl;
	std::cout << "p2:" << p2 << " " << std::hex << (int)&p2 << std::endl;
}


#define DEFAULT_SIZE 1024

class tcp_packet_original
{
public:
	tcp_packet_original();
	tcp_packet_original(tcp_packet_original& o);
	tcp_packet_original& operator=(tcp_packet_original& o);
	void reset();
	bool is_done();

	char* data;//[head:1byte][checknum:1byte][bodylen:4byte][body:nbyte(0-65535)]
	char head;//[head:1byte]packet head
	char checknum;//[checknum:1byte]packet check num : from server
	char calchecknum;//[checknum:1byte]packet check num : from calculate

	/*
		//整数转换为二进制
		//例如：336 转换为二进制：0000 0001 0101 0000 保存位字节为：0000 0010 1101 0000
		//例如：44022 转换为二进制：0000 0000 1010 1011 1111 0110 保存位字节为：0000 0010 1101 0111 1111 0110
		//charbodysize[4] = 1111 0110|1101 0111|0000 0010|0000 0000
		//二进制转换为整数
		//例如：101010000 转换为整数：336
	*/
	char charbodysize[4];//maybe 1-3
	uint8_t charbodysizelen;//charbodysize size

	uint32_t bodysize;//[body:nbyte]by bytes
	uint32_t datapos;//body current position. if datapos == bodysize_ packet is is_done.
private:
	void swap(tcp_packet_original& l, tcp_packet_original& r);
};
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

char* test_make_packet()
{
	//head[0|0|0|0|flag|flag|flag|flag]
	//checknum
	//bodylen[00000000][00000000][00000000][10000000]
	//body
	return nullptr;
}



int calculate_body_size(char* bodychar, int len) {
	return 0;
}


bool is_high_pos_byte(const char data);
bool get_body_size(const char* cbodysize, const uint8_t cbodysizelen, uint32_t& bodysize, char& checkcode);
bool get_char_body_size(const uint32_t bodysize, char* cbodysize, uint8_t& cbodysizelen);


#include <sstream>
std::string string_to_hex(const std::string& str)
{
	const std::string hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (std::string::size_type i = 0; i < str.size(); ++i)
		ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf];
	return ss.str();
}

std::string hex_to_string(const std::string& hex)
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



bool test_recv_data(char* recv_data, int len, int& really_len) {

	//单个协议
	if (false)
	{
		*recv_data = 'c';
		*(recv_data + 1) = 'e';
		const char* body = "tt2342234234234";
		uint32_t value = std::strlen(body) * sizeof(char);
		uint8_t bodylen = 0;
		get_char_body_size(value, recv_data + 2, bodylen);
		std::memcpy(recv_data + 2 + bodylen, body, value);
		really_len = 2 + bodylen + value;
	}
	//2个协议
	if (false)
	{
		*recv_data = 'c';
		*(recv_data + 1) = 'e';
		const char* body = "tt";
		uint32_t value = std::strlen(body) * sizeof(char);
		uint8_t bodylen = 0;
		get_char_body_size(value, recv_data + 2, bodylen);
		std::memcpy(recv_data + 2 + bodylen, body, value);
		really_len = 2 + bodylen + value;
	
		*(recv_data + really_len) = 'd';
		*(recv_data + 1 + really_len) = 'g';
		const char* body2 = "tteeer000iiirtt";
		value = std::strlen(body2) * sizeof(char);
		bodylen = 0;
		get_char_body_size(value, recv_data + 2 + really_len, bodylen);
		std::memcpy(recv_data + 2 + bodylen + really_len, body2, value);
		really_len += 2 + bodylen + value;
	}
	//半个协议(报文长度部分)
	if (false)
	{
		static int control = 0;
		static int relly_len_test = 0;
		static int relly_len_1 = 0;
		if (0 == control)
		{
			*recv_data = 'c';
			*(recv_data + 1) = 'e';
			const char* body = "ttsjdfiosfjse;lisje;lfijse;lfijse;lfijse;lfijse;lfijs;lefijs;leifjsl;eijf;sliejf;sleijfsfnisenfsejfi"
				"ttsjdfiosfjse;lisje;lfijse;lfijse;lfijse;lfijse;lfijs;lefijs;leifjsl;eijf;sliejf;sleijfsfnisenfsejfi"
				"ttsjdfiosfjse;lisje;lfijse;lfijse;lfijse;lfijse;lfijs;lefijs;leifjsl;eijf;sliejf;sleijfsfnisenfsejfi"
				"ttsjdfiosfjse;lisje;lfijsrfdoshejfir";
			uint32_t value = std::strlen(body) * sizeof(char);
			uint8_t bodylen = 0;
			get_char_body_size(value, recv_data + 2, bodylen);
			std::memcpy(recv_data + 2 + bodylen, body, value);
			relly_len_test = 2 + bodylen + value;
			relly_len_1 = really_len = 2 + 1;
			control = 1;
		}
		else if (1 == control)
		{
			really_len = relly_len_test - relly_len_1;
			std::memcpy(recv_data, recv_data + relly_len_1, really_len);
		}
	}
	//半个协议(报文内容部分)
	if (true)
	{
		static int control = 0;
		static int relly_len_test = 0;
		static int relly_len_1 = 0;
		if (0 == control)
		{
			*recv_data = 'c';
			*(recv_data + 1) = 'e';
			const char* body = "ttsjdfiosfjse;lisje;lfijse;lfijse;lfijse;lfijse;lfijs;lefijs;leifjsl;eijf;sliejf;sleijfsfnisenfse100"
				"ttsjdfiosfjse;lisje;lfijse;lfijse;lfijse;lfijse;lfijs;lefijs;leifjsl;eijf;sliejf;sleijfsfnisenfse200"
				"ttsjdfiosfjse;lisje;lfijse;lfijse;lfijse;lfijse;lfijs;lefijs;leifjsl;eijf;sliejf;sleijfsfnisenfse300"
				"ttsjdfiosfjse;lisje;lfijsrfdoshejfir";
			uint32_t value = std::strlen(body) * sizeof(char);
			uint8_t bodylen = 0;
			get_char_body_size(value, recv_data + 2, bodylen);
			std::memcpy(recv_data + 2 + bodylen, body, value);
			relly_len_test = 2 + bodylen + value;
			relly_len_1 = really_len = 2 + 100;
			control = 1;
		}
		else if (1 == control)
		{
			really_len = relly_len_test - relly_len_1;
			std::memcpy(recv_data, recv_data + relly_len_1, really_len);
			control = 2;
		}
	}

	return true;
}

void run_for_server()
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
		//if (user_quit_app_.load())
		//	break;
		int really_size = 0;
		int recv_data_pos = 0;
		if (test_recv_data(recv_data, DEFAULT_SIZE, really_size)) {
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

#include <bitset>

int64_t get_mask(int8_t pos)
{
	//from 0 to 63
	return ~((int64_t)1 << pos);
}
int64_t get_bit(int8_t pos)
{
	//from 0 to 63
	return (int64_t)1 << pos;
}

std::string num_to_binary(int64_t num)
{
	std::string binary;
	int64_t tmp = num;
	while (tmp > 0)
	{
		int64_t nextmp = tmp / 2;
		char c = tmp % 2 == 1 ? '1' : '0';
		binary += c;
		tmp = nextmp;
	}
	return std::move(binary);
}

uint32_t decodeBodyLen(char * datas, uint8_t lenBytes, char& checkCode)
{
	int digit = 0;
	int length = 0;
	int multiplier = 1;
	for (int i = 0; i < lenBytes; ++i)
	{
		digit = datas[i];
		checkCode = checkCode ^ datas[i];

		length += (digit & 127) * multiplier;
		multiplier *= 128;
	}

	return length;
}


void GenerateVarBodyChar(char * lenBuf, uint32_t len, uint8_t& bytesLen)
{
	uint8_t digit;
	uint8_t pos = 0;

	do {
		digit = len % 128;
		len = len / 128;
		if (len > 0) {
			digit |= 0x80;
		}
		lenBuf[pos++] = digit;

	} while (len > 0);

	bytesLen = pos;
}

void test_body_size()
{
	//body_size
	//int i = 1;
	//int ret = i << 7;
	//int ret2 = ret;

	int test_value = 336;



	//整数转换为二进制
	//例如：336 转换为二进制：0000 0001 0101 0000 保存位字节为：0000 0010 1101 0000
	//例如：44022 转换为二进制：0000 0000 1010 1011 1111 0110 保存位字节为：0000 0010 1101 0111 1111 0110

	//方法1：
	if (false)
	{
		std::bitset<32> bodylen(test_value);
		std::bitset<32> bodylenbybyte;
		int cout = bodylen.count();
		int cout2 = bodylen.size();

		int highpos = 0;
		for (int i = 0; i < cout2 - 4; ++i)
		{
			highpos = i;
			if (bodylen.test(i))
			{
				--cout;
				if (0 == cout)
				{
					break;
				}
			}
		}

		int offset = 0;
		for (int i = 0; i < cout2 - 4 && i < highpos + 1; ++i)
		{
			if (0 == i % 7 && 0 != i)
			{
				bodylenbybyte.set(i + offset, true);
				++offset;
			}
			bodylenbybyte.set(i + offset, bodylen.test(i));
		}
		std::cout << "cout: " << bodylenbybyte.count() << "string: " << bodylenbybyte.to_string() << std::endl;
	}

	//方法2：
	if (false)
	{
		//1.需要几个字节
		//2.字节内容
		int8_t highpos = 0;
		for (int8_t i = 0; i < 64; ++i)
		{
			int64_t ret = get_bit(i);//test
			if (0 != (test_value & get_bit(i)))
			{
				highpos = i;
			}
		}
		int bytelen = (highpos + 1) % 8 == 0 
			? (highpos + 1) / 8 
			: (highpos + 1) / 8 + 1;




		int64_t a = 0;
	}

	//方法3：
	if (false)
	{
		std::string&& binary = num_to_binary(test_value);
		size_t binarylen = binary.size();
		int bytelen = binarylen % 8 == 0
			? binarylen / 8
			: binarylen / 8 + 1;
		
		int offset = 0;
		int bytelennew = binarylen % 7 == 0
			? binarylen / 7
			: binarylen / 7 + 1;
		char* binarynew = new char[bytelennew];
		std::memset(binarynew, '0', bytelennew);
		std::string binaryconvert;
		int i = 0;
		for (auto iter=binary.rbegin(); iter!=binary.rend(); ++iter)
		{
			if (0 == i % 7 && 0 != i)
			{
				binaryconvert += '1';
				++offset;
			}
			binaryconvert += *iter;
		}
		for (int i = 0; i < binarylen; ++i)
		{
			if (0 == i % 7 && 0 != i)
			{
				//bodylenbybyte.set(i + offset, true);
				++offset;
			}
		}
	}


	//二进制转换为整数
	//例如：101010000 转换为整数：336


}

bool is_high_pos_byte(const char data)
{
	uint8_t d = data;
	if (d & 0x80)
		return false;
	else
		return true;
}

bool get_body_size(const char* cbodysize, const uint8_t cbodysizelen, uint32_t& bodysize, char& checkcode)
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
bool get_char_body_size(const uint32_t bodysize, char* cbodysize, uint8_t& cbodysizelen)
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


void test_num_and_char()
{
	uint32_t test_value = 44022;



	//整数转换为二进制
	//例如：336 转换为二进制：0000 0001 0101 0000 保存位字节为：0000 0010 1101 0000
	//例如：44022 转换为二进制：0000 0000 1010 1011 1111 0110 保存位字节为：0000 0010 1101 0111 1111 0110

	char cbodysize[4] = { 0 };
	uint8_t cbodysizelen = 0;
	get_char_body_size(test_value, cbodysize, cbodysizelen);


	//二进制转换为整数
	//例如：101010000 转换为整数：336
	uint32_t ret = 0;
	char checkcode;
	get_body_size(cbodysize, cbodysizelen, ret, checkcode);

	int a = 9;
}


int main()
{
	//test_async();
	//test_async_result();
	run_for_server();
    std::cout << "Hello World!\n"; 
	system("pause");
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
