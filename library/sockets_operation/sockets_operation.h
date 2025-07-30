#pragma once
#include "sockets_common.hpp"
#include <functional>
#include <memory>
#include <string>

class PassiveClientBase;
struct ServerWorkers {
	using AcceptCallback = std::function<void(PassiveClientBase*)>;
	using ReceivingCallback = std::function<void(PassiveClientBase*, const std::string& data)>;
	using CloseClientCallBack = std::function<void(PassiveClientBase*)>;
	using BroadCastProcessor = std::function<std::string(PassiveClientBase* who_broadcast, const std::string&)>;
	AcceptCallback accept_callback;
	ReceivingCallback receiving_callback;
	CloseClientCallBack close_client_callback;
	BroadCastProcessor broadcast_callback;
	bool broadcast_enabled { false };
};

struct ClientWorker {
	using ReceivingCallback = std::function<void(const std::string& str)>;
	ReceivingCallback receiving_callback;
};

class ServerSocket;
class ServerSocketInterface {
public:
	ServerSocketInterface(SocketsCommon::ServerProtocolSettings
	                          settings);
	~ServerSocketInterface();
	/**
	 * @brief listen to the port and ready to make server
	 *
	 * @param port
	 * @param max_accept
	 * @param settings
	 */
	void listen_at_port(const int port, const int max_accept,
	                    const ServerWorkers& worker);

private:
	std::unique_ptr<ServerSocket> serverImpl;
};

class ClientSocket;
class ClientInterface {
public:
	ClientInterface(const SocketsCommon::ClientProtocolSettings& settings);
	~ClientInterface();
	void connect_to(const std::string& host, const int target_port);
	void async_send_raw_data(const std::string& datas);
	void async_receive_from(const ClientWorker& worker);
	void close_self();

private:
	ClientInterface(const ClientInterface&) = delete;
	ClientInterface& operator=(const ClientInterface&) = delete;
	std::unique_ptr<ClientSocket> clientImpl;
};
