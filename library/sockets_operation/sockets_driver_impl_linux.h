#pragma once
#include "passive_client_socket.hpp"
#include "sockets_common.hpp"
#include "sockets_driver_impl.h"
#include <arpa/inet.h>
#include <atomic>
#include <thread>
#include <unordered_set>
#include <vector>
class LinuxServerSocket : public ServerSocket {
public:
	LinuxServerSocket() = default;
	~LinuxServerSocket() override {
		close_self();
	}

	void make_settings(const SocketsCommon::ServerProtocolSettings& settings) override;
	void listen_up(int port, int max_accept) override;
	void start_workloop(const ServerWorkers& worker) override;
	void close_self() override;

private:
	void init_epolls();
	void handle_new_connections();
	void close_target_client(int client_fd);
	void react_clients(int client_fd);
	inline bool is_acceptable_of_io() const {
		return socket_fd > 0;
	}
	bool already_listen_up { false };
	bool is_settings { false };
	bool error_state { false };
	int socket_fd { -1 };
	int epfd { -1 };
	int protocol_family;
	int socket_type;
	int max_epoll_contains;
	int buffer_cached_size;
	std::atomic<bool> shell_terminate { false };
	ServerWorkers internal_worker;
	std::unordered_set<LinuxLightPassiveClient> clients;
	std::vector<char> buffer;
};

class LinuxClientSocket : public ClientSocket {
public:
	LinuxClientSocket() { };
	~LinuxClientSocket() override;
	void make_settings(const SocketsCommon::ClientProtocolSettings& settings) override;
	void connect_to(const std::string& host, const int target_port) override;
	void async_send_to(const std::string& datas) override;
	void async_receive_from(const ClientWorker& worker) override;
	void close_self() override;

private:
	void sync_listen_loop(); ///< you can choose to put it async
	std::thread run_thread;
	std::atomic<bool> shell_terminate { false };
	ClientWorker worker;
	bool is_settings = false;
	bool is_running = false;
	int epoll_max_fds;
	std::vector<char> buffer;
	int cached_buffer_size;
	int target_server_fd { -1 };
	int sock_family;
	int epoll_fd { -1 };
};
