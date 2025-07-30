#include "sockets_common.hpp"
#include "sockets_operation.h"
#include <exception>
#include <iostream>
#include <ostream>
#include <string>

int main() {
	// try connect
	SocketsCommon::ClientProtocolSettings settings;
	settings.settings = {};
	settings.epoll_max_contains = 64;
	settings.buffer_length = 1024;
	ClientWorker worker;
	int i = 0;
	worker.receiving_callback = [&i](const std::string& value) {
		std::cout << "[" << i << "] " << value << std::endl;
		i++;
	};

	try {
		ClientInterface interface { settings };
		interface.connect_to("127.0.0.1", 12345);
		interface.async_send_raw_data("Hello You!");
		interface.async_receive_from(worker);
		// interface.close_self();
		std::string local;
		while (std::getline(std::cin, local)) {
			interface.async_send_raw_data(local);
		}
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}