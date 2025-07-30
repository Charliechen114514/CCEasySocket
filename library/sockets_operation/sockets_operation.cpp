#include "sockets_operation.h"
#include "sockets_common.hpp"
#include "sockets_driver_impl.h"

#ifdef __linux__
#include "sockets_driver_impl_linux.h"
#else
#endif

class SocketServerFactory {
public:
	static ServerSocket* queryDriver() {
#ifdef __linux__
		return new LinuxServerSocket();
#else
		throw "UnImplemented Platform!";
#endif
	}
};

class SocketClientFactory {
public:
	static ClientSocket* queryDriver() {
#ifdef __linux__
		return new LinuxClientSocket();
#else
		throw "UnImplemented Platform!";
#endif
	}
};

ServerSocketInterface::ServerSocketInterface(SocketsCommon::ServerProtocolSettings
                                                 settings)
    : serverImpl(SocketServerFactory::queryDriver()) {
	serverImpl->make_settings({ settings });
}

ServerSocketInterface::~ServerSocketInterface() = default;

void ServerSocketInterface::listen_at_port(const int port, const int max_accept,

                                           const ServerWorkers& worker) {

	serverImpl->listen_up(port, max_accept);
	serverImpl->start_workloop(worker);
}

ClientInterface::ClientInterface(const SocketsCommon::ClientProtocolSettings& settings)
    : clientImpl(SocketClientFactory::queryDriver()) {
	clientImpl->make_settings(settings);
}
ClientInterface::~ClientInterface() = default;
void ClientInterface::connect_to(const std::string& host, const int target_port) {
	clientImpl->connect_to(host, target_port);
}

void ClientInterface::async_send_raw_data(const std::string& datas) {
	clientImpl->async_send_to(datas);
}

void ClientInterface::async_receive_from(const ClientWorker& worker) {
	clientImpl->async_receive_from(worker);
}

void ClientInterface::close_self() {
	clientImpl->close_self();
}