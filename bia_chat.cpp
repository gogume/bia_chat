#include "bia_chat.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>
#endif

std::vector<int> clients;
std::mutex clients_mutex;

void broadcast(const std::string &message, int sender_socket) {
  std::lock_guard<std::mutex> lock(clients_mutex);

  std::cout << "Clients: " << clients.size() << std::endl;
  for (int client : clients) {
    if (client != sender_socket) { // optional: don't echo back
      send_to_client(client, message);
    }
  }
}

void send_to_client(int client_socket, const std::string &message) {
  send(client_socket, message.c_str(), message.size(), 0);
}


void handle_client(int client_socket, const std::function<void(std::string, int)> &on_message) {
  char buffer[1024];

  while (true) {
    ssize_t bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
      std::cout << "Client disconnected\n";
      break;
    }

    std::string message(buffer, bytes);
    on_message(message, client_socket);
  }

  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_socket),
                  clients.end());
  }

  close_socket(client_socket);
}

std::string get_local_ip() {
#ifdef _WIN32
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    struct hostent *host_info = gethostbyname(hostname);
    if (host_info && host_info->h_addr_list[0]) {
      return inet_ntoa(*(struct in_addr *)host_info->h_addr_list[0]);
    }
  }
  return "127.0.0.1";
#else
  struct ifaddrs *ifaddr, *ifa;

  if (getifaddrs(&ifaddr) == -1) {
    return "unknown";
  }

  std::string ip = "unknown";

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr)
      continue;

    // Only IPv4
    if (ifa->ifa_addr->sa_family == AF_INET) {
      // Skip loopback
      if (ifa->ifa_flags & IFF_LOOPBACK)
        continue;

      char host[NI_MAXHOST];
      void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

      inet_ntop(AF_INET, addr, host, NI_MAXHOST);

      ip = host;
      break; // take first non-loopback
    }
  }

  freeifaddrs(ifaddr);
  return ip;
#endif
}

int start_server() {
#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  int server_fd;
  struct sockaddr_in address;

  const int PORT = 8080;
  const int MAX_CLIENTS = 2;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 5);

  std::string ip = get_local_ip();
  std::cout << "Server is available at " << ip << ":" << PORT << std::endl;

  return server_fd;
}

void close_server(int server_fd) {
  close_socket(server_fd);
}


int accept_connect_client(int server_fd, const std::function<void(std::string, int)> &on_message) {
  int client_socket = accept(server_fd, nullptr, nullptr);

	{
			std::lock_guard<std::mutex> lock(clients_mutex);
			clients.push_back(client_socket);
	}

  // Create a detached thread for each client
  std::thread t(handle_client, client_socket, on_message);
  t.detach();

  return client_socket;
}

void receive_loop(int sock) {
  char buffer[1024];

  while (true) {
    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
      std::cout << "Disconnected from server\n";
      break;
    }

    buffer[bytes] = '\0';
      
    std::cout << buffer << std::endl;
    std::cout << "> " << std::flush;
  }
}

void send_loop(int sock) {
  std::string input;

  while (true) {
    std::cout << "> ";
    std::getline(std::cin, input);

    send(sock, input.c_str(), input.size(), 0);
  }
}

void connect_client(std::string IP, int PORT) {
#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in serv_addr{};
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, IP.c_str(), &serv_addr.sin_addr);

  connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr));

  std::thread t1(receive_loop, sock);
  std::thread t2(send_loop, sock);

  t1.join();
  t2.join();

  close_socket(sock);
}