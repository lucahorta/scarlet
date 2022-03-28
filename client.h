#pragma once
#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include "ip_address.h"
#include <vector>
#include <ctime>
#include "packet.h"

constexpr auto largest_packet_size = 0x10000;

class c_client {
public:
	//variables
	SOCKET socket;
	ip_address_t addr;
	int port;	
	std::clock_t last_request;

	c_client ( uintptr_t socket, const ip_address_t& addr, int port, std::clock_t last_request );
	bool operator==( const c_client& other ) const;
	bool operator!=( const c_client& other ) const;
	bool send ( const std::vector<uint8_t>& buffer ) const;
	bool send ( c_packet packet ) const;
	bool recv ( std::vector<uint8_t>& buffer ) const;
	bool is_connected ( ) const;
	bool close ( );
};