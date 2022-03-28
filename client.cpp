#include "client.h"

//constructors
c_client::c_client ( uintptr_t socket, const ip_address_t& addr, int port, std::clock_t last_request ) : socket ( socket ), addr ( addr ), port ( port ), last_request ( last_request ) {}
//operators
bool c_client::operator ==( const c_client& other ) const {
	return this->socket == other.socket;
}
bool c_client::operator !=( const c_client& other ) const {
	return this->socket != other.socket;
}
//functions
bool c_client::send ( const std::vector<uint8_t>& buffer ) const {
	size_t bytes_remaining = buffer.size ( );

	auto data = buffer.data ( );

	while ( bytes_remaining > 0 ) {
		const auto result = ::send ( socket, reinterpret_cast< const char* >( data ), bytes_remaining, 0 );

		if ( result < 1 ) {
			std::cout << "[!] send failed with error code: " << WSAGetLastError ( ) << std::endl;
			return false;
		}

		data += result;
		bytes_remaining -= result;
	}

	return true;
}

bool c_client::send (c_packet packet ) const {
	return send ( packet.get_vec ( ) );
}

bool c_client::recv ( std::vector<uint8_t>& buffer ) const {
	size_t bytes_available = 0;
	size_t bytes_read = 0;

	do {
		ioctlsocket ( socket, FIONREAD, reinterpret_cast< u_long* >( &bytes_available ) );

		if ( buffer.size ( ) > largest_packet_size )
			return false;

		if ( bytes_available == 0 )
			break;

		if ( buffer.size ( ) < buffer.size ( ) + bytes_available )
			buffer.resize ( buffer.size ( ) + bytes_available );

		const auto result = ::recv ( this->socket, reinterpret_cast< char* >( buffer.data ( ) + bytes_read ), bytes_available, 0 );

		if ( result < 1 ) {
			std::cout << "[!] recv failed with error code: " << WSAGetLastError ( ) << std::endl;
			return false;
		}

		bytes_read += result;
	} while ( bytes_available != 0 );

	return true;
}

bool c_client::is_connected ( ) const {
	return ::send ( socket, nullptr, 0, 0 ) == 0;
}

bool c_client::close ( ) {
	if ( socket != 0 ) {
		bool r = closesocket ( socket );
		socket = 0;
		return r;
	}
}