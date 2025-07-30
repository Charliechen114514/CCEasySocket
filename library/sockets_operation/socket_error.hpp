#pragma once
#include <stdexcept>
struct socket_internal_error_create : public std::runtime_error {
	socket_internal_error_create()
	    : std::runtime_error("Socket Create Error!") {
	}
};

struct socket_dead_error : public std::runtime_error {
	socket_dead_error()
	    : std::runtime_error("Socket is dead! you should invoke listen up first") {
	}
};

struct bind_error : public std::runtime_error {
	bind_error()
	    : std::runtime_error("Bind Error, maybe same instance has been running!") {
	}
};

struct listen_error : public std::runtime_error {
	listen_error()
	    : std::runtime_error("Listen Error") {
	}
};

struct configure_error : public std::runtime_error {
	configure_error()
	    : std::runtime_error("Configure Error!") {
	}
};

struct server_unreachable : public std::runtime_error {
	server_unreachable()
	    : std::runtime_error("server unreachable Error!") {
	}
};