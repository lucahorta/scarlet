#pragma once
#include <winsock2.h>
#include <string>
#include <WS2tcpip.h>

class ip_address_t {
public:
	ip_address_t ( const ip_address_t& c ) {
		inet_ret = c.inet_ret;
		binary_address = c.binary_address;
		string_address = c.string_address;
	}

	ip_address_t ( ) = default;

	ip_address_t ( const std::string& address ) : binary_address ( 0 ), string_address ( address ) {
		inet_ret = inet_pton ( AF_INET, address.c_str ( ), &this->binary_address );
	}

	ip_address_t ( IN_ADDR address ) {
		binary_address = *( uint32_t* ) &address;
		string_address.resize ( INET_ADDRSTRLEN );
		inet_ret = (uint64_t)inet_ntop ( AF_INET, &address, ( char* ) string_address.data ( ), INET_ADDRSTRLEN );
		string_address.resize ( strlen ( string_address.data ( ) ) );
	}

	const std::string& to_string ( ) const {
		return string_address;
	}

	bool is_valid ( ) const {
		return this->binary_address != 0 && inet_ret > 0;
	}

	uint32_t address ( ) const {
		return this->binary_address;
	}

	uint32_t binary_address;
	std::string string_address;
	uint64_t inet_ret = -1;
};