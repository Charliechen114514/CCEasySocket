#pragma once

namespace SocketsCommon {
enum class IPVersionType : unsigned char {
	IPv4,
	IPv6
};

enum class TransferType : unsigned char {
	STREAM,
	DATA
};

struct ProtocolLevelSettings {
	static constexpr SocketsCommon::IPVersionType ip_versionType_v = SocketsCommon::IPVersionType::IPv4;
	static constexpr SocketsCommon::TransferType transferType_v = SocketsCommon::TransferType::STREAM;
};

struct ProtocolSettings {
	virtual ~ProtocolSettings() = default;
	SocketsCommon::ProtocolLevelSettings settings;
};

struct ServerProtocolSettings : public ProtocolSettings {
	int buffer_length { 1024 };
	int epoll_max_contains { 64 };
};

struct ClientProtocolSettings : public ProtocolSettings {
	int epoll_max_contains { 64 };
	int buffer_length { 1024 };
};

};
