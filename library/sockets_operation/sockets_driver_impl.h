#pragma once
#include "sockets_common.hpp"
#include "sockets_operation.h"

/**
 * @brief driver interface
 *
 */
class SocketDriver {
public:
	virtual ~SocketDriver() = default;
	virtual void close_self() = 0;
};

class ServerSocket : public SocketDriver {
public:
	virtual ~ServerSocket() = default;
	virtual void make_settings(const SocketsCommon::ServerProtocolSettings& settings) = 0;
	virtual void listen_up(int port, int max_accept) = 0;
	virtual void start_workloop(const ServerWorkers& worker) = 0;
};

class ClientSocket : public SocketDriver {
public:
	virtual ~ClientSocket() override = default;
	virtual void make_settings(const SocketsCommon::ClientProtocolSettings& settings) = 0;
	virtual void connect_to(const std::string& str, const int port) = 0;
	virtual void async_send_to(const std::string& datas) = 0;
	virtual void async_receive_from(const ClientWorker& worker) = 0;
};
