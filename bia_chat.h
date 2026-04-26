#pragma once

#include <string>
#include <functional>

#ifdef _WIN32
  #ifdef BIA_CHAT_EXPORTS
    #define BIA_CHAT_API __declspec(dllexport)
  #else
    #define BIA_CHAT_API __declspec(dllimport)
  #endif
#else
  #define BIA_CHAT_API
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
inline void close_socket(int sock) { closesocket(sock); }
#else
#include <sys/types.h>
#include <unistd.h>
inline void close_socket(int sock) { close(sock); }
#endif

BIA_CHAT_API int start_server();
BIA_CHAT_API void close_server(int server_fd);
BIA_CHAT_API int accept_connect_client(int server_fd, const std::function<void(std::string, int)> &on_message);
// BIA_CHAT_API void broadcast(const std::string &message, int sender_socket);
BIA_CHAT_API void connect_client(std::string IP, int PORT);

BIA_CHAT_API void send_to_client(int client_socket, const std::string &message);