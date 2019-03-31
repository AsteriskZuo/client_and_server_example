#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <memory>
#include <future>
#include <string>
#include <atomic>

template<typename T>
class task_queue
{
public:
	explicit task_queue();
	void push_task(const T& task);
	T& obtain_task();
	void quit();

protected:
	void wait();
	void wakeup();
	bool is_quit() { return user_quit_app_.load(); }

private:
	std::queue<T> tasks_;
	std::mutex mutex_;
	std::condition_variable condition_;
	std::atomic_bool user_quit_app_;//0.not quit 1.quit
};

template<typename T>
void task_queue<T>::quit()
{
	user_quit_app_.store(true);
}

template<typename T>
task_queue<T>::task_queue()
{
	user_quit_app_.store(false);
}

template<typename T>
void task_queue<T>::wakeup()
{
	condition_.notify_one();//wakeup self
}

template<typename T>
void task_queue<T>::wait()
{
	std::unique_lock<std::mutex> lock(mutex_);
	condition_.wait(lock, std::bind(&task_queue::is_quit, this));
}

template<typename T>
T& task_queue<T>::obtain_task()
{
	if (0 == tasks_.size())
		wait();
	if (tasks_.size())
	{
		T& task = tasks_.front();
		tasks_.pop();
		return task;
	}
	return *std::shared_ptr<T>();
}

template<typename T>
void task_queue<T>::push_task(const T& task)
{
	tasks_.push(task);
	wakeup();
}

class tcp_info
{
public:
	std::string host;
	std::string port;
protected:
private:
};

class tcp_packet_info
{
public:
	int seqid;
	int ptype;//tcp packet type
	int direction;//0.send 1.recv
	int qos;//network quality
	int scode;//status code
	char* payload;//
	int payloadsize;//
	void operator()();
};
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

struct send_msg_info
{
	int seqid;
	char* data;//[head:1byte][checknum:1byte][bodylen:4byte][body:nbyte]
	int datasize;//by bytes
	void operator()();
};
struct recv_msg_info
{
	int seqid;
	char* data;//[head:1byte][checknum:1byte][bodylen:4byte][body:nbyte]
	int datasize;//by bytes
	void operator()();
};

class tcp_socket;
class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
public:
	tcp_connection();
	~tcp_connection();

	void init();
	void uninit();
	void connect(const tcp_info& info);
	void disconnect();
	void quit();
	void send_msg(const send_msg_info& info);//from app
	void recv_msg(const recv_msg_info& info);//from server

protected:
	void start();//start workers
	void stop();//end workers
	void start_internal();
	void run_for_send_msg();
	void run_for_recv_msg();
	void run_for_control();
	void start_recv();
	void stop_recv();
	void start_recv_internal();
	void run_for_server();

protected:
	bool _connect(const tcp_info& info);
	void _disconnect();

private:
	bool is_high_pos_byte(const char data);
	bool get_body_size(const char* cbodysize, const uint8_t cbodysizelen, uint32_t& bodysize, char& checkcode);
	bool get_char_body_size(const uint32_t bodysize, char* cbodysize, uint8_t& cbodysizelen);
	std::string string_to_hex(const std::string& str);
	std::string hex_to_string(const std::string& hex);
private:
	std::thread* send_msg_worker_;//send msg
	std::thread* recv_msg_worker_;//receive msg
	std::thread* timeout_monitor_;//timeout timer
	std::thread* recv_msg_from_server;//recv msg from server
	std::future<void> workers_result_;//
	std::future<void> recv_result_;//

	tcp_info tcp_info_;

	std::atomic_bool is_init_;
	std::atomic_int8_t connect_status_;//0.disconnected 1.connecting 2.connected
	std::atomic_bool worker_status_;//0.not work 1.working
	std::atomic_bool user_quit_app_;//0.not quit 1.quit

	task_queue<std::shared_ptr<send_msg_info>> send_msg_tasks_;
	task_queue<std::shared_ptr<recv_msg_info>> recv_msg_tasks_;
	std::shared_ptr<tcp_socket> socket_;
};

