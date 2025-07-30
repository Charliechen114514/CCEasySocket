#pragma once
#include <functional>
class PassiveClientBase {
public:
	virtual ~PassiveClientBase() = default;
	// this is a light weight client design for the
	// collected clients from server
};

#ifdef __linux__

struct LinuxLightPassiveClient : public PassiveClientBase {
	LinuxLightPassiveClient(int client_fd)
	    : passive_client_fd(client_fd) { }
	int passive_client_fd { -1 };
	bool operator==(const LinuxLightPassiveClient& other) const {
		return passive_client_fd == other.passive_client_fd;
	}
};

namespace std {
template <>
struct hash<LinuxLightPassiveClient> {
	std::size_t operator()(const LinuxLightPassiveClient& client) const noexcept {
		return std::hash<int>()(client.passive_client_fd);
	}
};
}

#endif