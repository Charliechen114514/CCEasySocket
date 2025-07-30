#include "passive_client_socket.hpp"
#include "sockets_common.hpp"
#include "sockets_operation.h"
#include <exception>
#include <iostream>
#include <ostream>
#include <string>
#include <sys/socket.h>
int main() {

	SocketsCommon::ServerProtocolSettings settings;
	settings.settings = {};
	settings.epoll_max_contains = 64;
	settings.buffer_length = 1024;
	ServerWorkers workers;
	workers.broadcast_enabled = true;
	workers.broadcast_callback = [](PassiveClientBase* sender, const std::string& data) -> std::string {
		LinuxLightPassiveClient* clients = dynamic_cast<LinuxLightPassiveClient*>(sender);
		return "[" + std::to_string(clients->passive_client_fd) + "]: " + data;
	};
	workers.accept_callback = [](PassiveClientBase* base) {
		LinuxLightPassiveClient* clients = dynamic_cast<LinuxLightPassiveClient*>(base);
		std::string echos = "Hello from server!";
		::send(clients->passive_client_fd, echos.data(), echos.size(), 0);
	};
	workers.close_client_callback = [](PassiveClientBase* base) {
		LinuxLightPassiveClient* clients = dynamic_cast<LinuxLightPassiveClient*>(base);
		std::cout << "Ok we close a client! with internal handle " << clients->passive_client_fd << std::endl;
	};
	workers.receiving_callback = [](PassiveClientBase* clients, std::string datas) {
		datas.resize(datas.size() + 1);
		datas[datas.size() - 1] = '\0';
		LinuxLightPassiveClient* _clients = dynamic_cast<LinuxLightPassiveClient*>(clients);
		std::cout << "Ok we have receive data from: " << datas << " from clients: " << _clients->passive_client_fd << std::endl;
		std::string echos = "Receiving datas: " + datas;
		::send(_clients->passive_client_fd, echos.data(), echos.size(), 0);
	};

	try {
		ServerSocketInterface interface { settings };
		interface.listen_at_port(12345, 5, workers);
	} catch (std::exception& e) {
		std::cerr << e.what();
	}

	while (1)
		;
}