add_library(SimpleLogger STATIC 
            simple_logger/simple_logger.cpp
            simple_logger/simple_logger.h)

add_library(SocketsLibrary STATIC 
            sockets_operation/sockets_operation.cpp
            sockets_operation/sockets_operation.h
            sockets_operation/socket_error.hpp
            sockets_operation/sockets_common.hpp
            sockets_operation/sockets_driver_impl.h
            sockets_operation/sockets_driver_impl_linux.cpp
            sockets_operation/sockets_driver_impl_linux.h
            sockets_operation/passive_client_socket.hpp)   

target_include_directories(SocketsLibrary PUBLIC sockets_operation)