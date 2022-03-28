#pragma once
#include <future>
#include <iostream>
#include <winsock2.h>
#include <string>
#include <WS2tcpip.h>
#include <map>
#include "anti_dos.h"
#include "client.h"
#include "logger.h"
#include "tasks.h"

class c_server {
public:
	c_logger log;
	std::mutex clients_mtx;
	c_tasks tasks;
	std::vector< c_client > clients;
	std::map<SOCKET, int> socket_index;
	std::function<bool ( const c_client client, const std::vector<uint8_t> buf )> request_callback;
private:
	fd_set master;

	SOCKET server_socket;
	ip_address_t ip_address;
	int port;

	bool initialize_wsa ( );
	bool bind ( );
	bool setsockopt ( );
	bool listen ( );

	void register_client ( SOCKET client_socket, const ip_address_t& client_addr, int client_port );
	void timeout_clients ( );
	void dispatch_callback ( c_client client, std::vector<uint8_t> buf );
	void dispatch_request ( SOCKET socket );
public:
	c_server ( const ip_address_t& _ip_address, uint32_t _port );
	//void start ( const ip_address_t& _ip_address, uint32_t _port );
	void serve_clients ( );
	std::vector<c_client>::iterator delete_client ( SOCKET socket );
	bool stop ( );
};