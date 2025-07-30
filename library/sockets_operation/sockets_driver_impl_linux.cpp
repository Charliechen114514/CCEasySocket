#include "sockets_driver_impl_linux.h"
#include "socket_error.hpp"
#include "sockets_operation.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
static const std::map<SocketsCommon::IPVersionType, int> ip_version_map = {
	{ SocketsCommon::IPVersionType::IPv4, AF_INET },
	{ SocketsCommon::IPVersionType::IPv6, AF_INET6 }
};

static const std::map<SocketsCommon::TransferType, int> transfer_protocolt_type = {
	{ SocketsCommon::TransferType::DATA, SOCK_DGRAM },
	{ SocketsCommon::TransferType::STREAM, SOCK_STREAM }
};

void LinuxServerSocket::make_settings(const SocketsCommon::ServerProtocolSettings& settings) {
	protocol_family = ip_version_map.at(settings.settings.ip_versionType_v);
	socket_type = transfer_protocolt_type.at(settings.settings.transferType_v);
	max_epoll_contains = settings.epoll_max_contains;
	buffer_cached_size = settings.buffer_length;
	buffer.resize(buffer_cached_size);
	is_settings = true;
}

void LinuxServerSocket::listen_up(int port, int max_accept) {
	try {
		if (!is_settings)
			throw configure_error();

		socket_fd = socket(protocol_family, socket_type, 0);
		if (!is_acceptable_of_io())
			throw socket_internal_error_create();

		struct sockaddr_in sock_addr;
		sock_addr.sin_family = protocol_family;
		sock_addr.sin_port = htons(port);
		sock_addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(
		        socket_fd,
		        (const struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in))
		    < 0) {
			throw bind_error();
		}

		if (listen(socket_fd, max_accept) < 0) {
			throw listen_error();
		}
	} catch (const std::exception& e) {
		error_state = true;
		throw e;
	}
	already_listen_up = true;
	init_epolls();
}

void LinuxServerSocket::start_workloop(const ServerWorkers& worker) {
	if (!already_listen_up) {
		throw socket_dead_error();
	}

	internal_worker = worker;
	std::vector<epoll_event> events(max_epoll_contains);
	while (!shell_terminate) {
		int nfds = epoll_wait(epfd, events.data(), max_epoll_contains, -1);
		for (int i = 0; i < nfds; ++i) {
			int current_fd = events[i].data.fd;
			if (current_fd == socket_fd) {
				handle_new_connections();
			} else {
				react_clients(current_fd);
			}
		}
	}
}

void LinuxServerSocket::close_self() {
	// clear the holding clients
	shell_terminate = true;
	for (const auto& client : clients) {
		::close(client.passive_client_fd);
	}
	clients.clear();
	if (epfd > 0)
		close(epfd);
	close(socket_fd);
}

void LinuxServerSocket::init_epolls() {
	int flags = fcntl(socket_fd, F_GETFL, 0);
	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
	epfd = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event ev = {};
	ev.events = EPOLLIN;
	ev.data.fd = socket_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &ev);
}

void LinuxServerSocket::react_clients(int current_fd) {
	ssize_t len;
	do {
		len = recv(current_fd, buffer.data(), buffer_cached_size, 0);
	} while (len < 0 && errno == EINTR);

	if (len > 0) {
		std::string received_data_copy(buffer.data(), len);
		if (internal_worker.receiving_callback) {

			LinuxLightPassiveClient client { current_fd };
			internal_worker.receiving_callback(&client, received_data_copy);
		}
		if (internal_worker.broadcast_enabled) {
			LinuxLightPassiveClient sender { current_fd };
			for (auto& client : clients) {
				if (client.passive_client_fd == current_fd)
					continue;
				LinuxLightPassiveClient client_handle { client.passive_client_fd };
				if (internal_worker.broadcast_callback) {
					std::string received_data = internal_worker.broadcast_callback(&sender, received_data_copy.data());
					internal_worker.receiving_callback(&client_handle, received_data);
				}
			}
		}
	} else if (len == 0) {
		close_target_client(current_fd);
	} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
		close_target_client(current_fd);
	}
}

void LinuxServerSocket::close_target_client(int fd) {
	clients.erase(LinuxLightPassiveClient(fd));
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
	::close(fd);
	LinuxLightPassiveClient client { fd };
	if (internal_worker.close_client_callback)
		internal_worker.close_client_callback(&client);
}

void LinuxServerSocket::handle_new_connections() {
	int client_fd = accept(socket_fd, nullptr, nullptr);
	if (client_fd < 0) {
		// unblockings
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
		throw std::runtime_error("Accept Error");
	}
	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	epoll_event cev { EPOLLIN, { .fd = client_fd } };
	epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
	LinuxLightPassiveClient new_client(client_fd);
	clients.insert(new_client); // OK, insert this
	if (internal_worker.accept_callback) {
		internal_worker.accept_callback(&new_client);
	}
}

LinuxClientSocket::~LinuxClientSocket() {
	close_self();
}

void LinuxClientSocket::make_settings(const SocketsCommon::ClientProtocolSettings& settings) {
	epoll_max_fds = settings.epoll_max_contains;
	sock_family = transfer_protocolt_type.at(settings.settings.transferType_v);
	cached_buffer_size = settings.buffer_length;
	buffer.resize(cached_buffer_size);
	is_settings = true;
}

void LinuxClientSocket::connect_to(
    const std::string& host, const int target_port) {
	if (!is_settings)
		throw configure_error();

	struct addrinfo hints {}, *res = nullptr;
	hints.ai_family = AF_UNSPEC; // IPv4 或 IPv6
	hints.ai_socktype = sock_family;
	// DNS Query is must, sync
	int err = getaddrinfo(host.c_str(), std::to_string(target_port).c_str(), &hints, &res);
	if (err != 0) {
		throw server_unreachable();
	}

	for (auto p = res; p != nullptr; p = p->ai_next) {
		target_server_fd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK, p->ai_protocol);
		if (::connect(target_server_fd, p->ai_addr, p->ai_addrlen) == 0) {
			break;
		} else if (errno == EINPROGRESS) {
			fd_set writefds;
			FD_ZERO(&writefds);
			FD_SET(target_server_fd, &writefds);

			timeval timeout {};
			timeout.tv_sec = 3;
			int res = select(target_server_fd + 1, nullptr, &writefds, nullptr, &timeout);
			if (res > 0 && FD_ISSET(target_server_fd, &writefds)) {
				int so_error = 0;
				socklen_t len = sizeof(so_error);
				getsockopt(target_server_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
				if (so_error == 0) {
					break;
				}
			}
		}
		close(target_server_fd);
		target_server_fd = -1;
	}

	freeaddrinfo(res);
	if (target_server_fd < 0)
		throw server_unreachable();

	epoll_fd = epoll_create1(0);

	epoll_event ev {};
	ev.events = EPOLLIN;
	ev.data.fd = target_server_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_server_fd, &ev);
	this->is_running = true;
	run_thread = std::thread([this]() {
		this->shell_terminate = false;
		this->sync_listen_loop();
	});
}
void LinuxClientSocket::async_send_to(const std::string& datas) {
	if (!is_running)
		throw server_unreachable();
	// we have set async method
	::send(target_server_fd, datas.data(), datas.size(), 0);
}

void LinuxClientSocket::async_receive_from(const ClientWorker& worker) {
	this->worker = std::move(worker);
}

void LinuxClientSocket::close_self() {
	shell_terminate = true;
	is_running = false;

	if (epoll_fd >= 0)
		close(epoll_fd);
	if (target_server_fd >= 0)
		close(target_server_fd);
	target_server_fd = -1;
	if (run_thread.joinable() && std::this_thread::get_id() != run_thread.get_id()) {
		run_thread.join();
	}
}

void LinuxClientSocket::sync_listen_loop() {
	std::vector<epoll_event> events(epoll_max_fds);
	while (!shell_terminate) {
		int n = epoll_wait(epoll_fd, events.data(),
		                   epoll_max_fds, 1000);

		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0)
			break;

		for (int i = 0; i < n; i++) {
			int current_fd = events[i].data.fd;
			if (current_fd != target_server_fd)
				continue;

			ssize_t len = ::recv(target_server_fd,
			                     buffer.data(), buffer.size(), 0);
			if (len > 0) {
				if (worker.receiving_callback)
					worker.receiving_callback(std::string(buffer.data(), len));
			} else if (len == 0) {
				// close socket
				shell_terminate = true;
				return;
			} else {
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					shell_terminate = true;
					return;
				}
			}
		}
	}
}
