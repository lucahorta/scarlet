#include <future>
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <random>
#include <mutex>
#include "server.h"
#include "packet.h"

/*
	things i dont like.
	- i cant remove timed out clients from the fd since select is making changes. this is handled by checking if the client was deleted in the callbacks, sort of dirty imo
	- i cant remove other callbacks w the same client since i dont have access to either the mutex or the queue from my handler.
	- if i have too many requests got by select i will timeout clients very infrequently. maybe have a cap on how many requests, do some sort of checking for source to see if im being dos'd
*/

bool c_server::initialize_wsa ( ) {
	WSADATA wsa_data;
	WSAStartup ( MAKEWORD ( 2, 2 ), &wsa_data );
	server_socket = socket ( AF_INET, SOCK_STREAM, 0 );

	log << "[+] socket created" << std::endl;
	return server_socket != INVALID_SOCKET;
}

bool c_server::bind ( ) {
	sockaddr_in sockaddr = {};

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons ( port );
	sockaddr.sin_addr.s_addr = ip_address.address ( );

	log << "[+] binding socket to " << ip_address.to_string ( ) << ":" << port << std::endl;
	return ::bind ( this->server_socket, reinterpret_cast< ::sockaddr* >( &sockaddr ), sizeof ( ::sockaddr_in ) ) == 0;
}

bool c_server::setsockopt ( ) {
	bool opt = true;
	log << "[+] setsockopt called" << std::endl;
	return ::setsockopt ( server_socket, SOL_SOCKET, TCP_NODELAY, (const char*)&opt, sizeof ( bool ) ) == 0; 
}

bool c_server::listen ( ) {
	log << "[+] listening on " << ip_address.to_string ( ) << ":" << port << std::endl;
	return ::listen ( this->server_socket, 1 ) == 0;
}

c_server::c_server ( const ip_address_t& _ip_address, uint32_t _port ) {
	ip_address = _ip_address;
	port = _port;

	tasks.start ( );

	if ( !initialize_wsa ( ) ) {
		log << "[!] wsa initialization failed! error code:" << WSAGetLastError ( ) << std::endl;
		return;
	}
	if ( !setsockopt ( ) ) {
		log << "[!] setsockopt failed! error code: " << WSAGetLastError ( ) << std::endl;
		return;
	}
	if ( !bind ( ) ) {
		log << "[!] socket binding failed! error code:" << WSAGetLastError ( ) << std::endl;
		return;
	}
	if ( !listen ( ) ) {
		log << "[!] listening failed! error code:" << WSAGetLastError ( ) << std::endl;
		return;
	}
	log << "[+] socket initialized successfully" << std::endl;
}

void c_server::register_client ( SOCKET client_socket, const ip_address_t& client_addr, int client_port ) {
	static c_anti_dos anti_dos;

	if ( client_socket == INVALID_SOCKET ) {
		log << "[!] a connection with a client with ip: " << client_addr.to_string ( ) << ":" << client_port << " failed with error code : " << WSAGetLastError ( ) << "!" << std::endl;
		return; /// care you might need to close the aborted connection.
	}

	if ( !anti_dos.is_request_valid ( client_addr ) ) {
		closesocket ( client_socket );
		log << "[!] client with ip: " << client_addr.to_string ( ) << ":" << client_port << " attemped to ddos the server, banned for 120 seconds!" << std::endl;
		return;
	}

	lg lck ( clients_mtx );
	log << "[+] client with ip: " << client_addr.to_string ( ) << ":" << client_port << " connected!" << std::endl;
	socket_index.emplace ( std::make_pair ( client_socket, clients.size ( ) ) );
	clients.push_back ( c_client ( client_socket, client_addr, client_port, std::clock() ) );
	FD_SET ( client_socket, &master );
}

void c_server::timeout_clients ( ) {
	lg lck ( clients_mtx );
	auto cur_time = std::clock ( );
	for ( auto it = clients.begin ( ); it != clients.end ( ); ) {
		auto client = *it;
		const auto index = socket_index [ client.socket ];
		if ( ( cur_time - client.last_request ) / CLOCKS_PER_SEC > 15 || !client.is_connected() ) { // takes more than 15 seconds to respond
			log << "[+] client with id " << index << (!client.is_connected ( ) ? " disconnected!" : " was forcibly timed out!") << std::endl;
			it = delete_client ( client.socket );
			continue;
		}
		it++;
	}
}

void c_server::dispatch_callback ( c_client client, std::vector <uint8_t> buf ) {
	{
		lg lck ( clients_mtx );
		if ( socket_index.count ( client.socket ) == 0 ) // function was in the list and a function w the same client dropped the client by returning false
			return;
	}
	if ( !request_callback ( client, buf ) ) {
		lg lck ( clients_mtx ); 
		log << "[+] client " << socket_index [ client.socket ] << " was disconnected by the provided callback function!" << std::endl;
		delete_client ( client.socket );
	}
}

void c_server::dispatch_request ( SOCKET socket ) {
	lg lck ( clients_mtx );

	if ( socket_index.count ( socket ) == 0 ) // socket was deleted because of inactivity/left but still has requests in the FD array
		return;

	auto& client = clients [ socket_index[ socket ] ];
	client.last_request = std::clock ( );
	std::vector< uint8_t > buf;	client.recv ( buf );

	if ( buf.empty ( ) || !client.is_connected ( ) ) {
		log << "[+] client " << socket_index [ socket ] << " send empty buffer and has been forcibly disconnected" << std::endl;
		delete_client ( socket );
		return;
	}
	void( c_server:: * dispatch_callback_ptr )( c_client, std::vector<uint8_t>) = &c_server::dispatch_callback;
	tasks.push_task ( ( void( * )( void*, c_client, std::vector<uint8_t> ) )( ( ( void*& ) dispatch_callback_ptr ) ), (void*)this, client, buf );
}

void c_server::serve_clients ( ) {
	SOCKADDR_IN client_addr_temp;
	auto client_addr_size = ( int ) sizeof ( client_addr_temp );

	FD_ZERO ( &master );
	FD_SET ( server_socket, &master );

	while ( true ) {
		clients_mtx.lock ( );
		auto copy = master;
		clients_mtx.unlock ( );

		auto future = std::async ( std::launch::async, [ & ] ( ) -> int {
			return select ( 0, &copy, nullptr, nullptr, nullptr );
		} );
		auto status = std::future_status::deferred;
		while ( status != std::future_status::ready ) { // while theres no new connections run the time out func
			timeout_clients ( );
			status = future.wait_for ( std::chrono::milliseconds ( 10 ) );
		}
		const auto connection_count = future.get ( );

		for ( int i = 0; i < connection_count; i++ ) {
			const auto& cur_socket = copy.fd_array [ i ];
			if ( cur_socket == server_socket ) {// new connection
				const auto client_socket = accept ( server_socket, reinterpret_cast< SOCKADDR* >( &client_addr_temp ), &client_addr_size );
				const auto client_port = client_addr_temp.sin_port;
				ip_address_t client_addr ( client_addr_temp.sin_addr );

				register_client ( client_socket, client_addr, client_port );
			}
			else {
				dispatch_request ( cur_socket );
			}
		}
	}
}

std::vector<c_client>::iterator c_server::delete_client ( SOCKET socket ) {
	const auto index = socket_index [ socket ];
	socket_index.erase ( socket_index.find ( socket ) );
	for ( auto& s_index : socket_index ) { // shift indexes appropriately.
		if ( s_index.second > index )
			s_index.second--;
	}
	FD_CLR ( socket, &master );
	closesocket ( socket );

	return clients.erase ( clients.begin() + index );
}

bool c_server::stop ( ) {
	tasks.stop ( );
	return closesocket ( server_socket );
}