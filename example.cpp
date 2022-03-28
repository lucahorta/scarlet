#include "tatouille.h"
#include <windows.h>
#include <string>

bool call_back ( const c_client client, std::vector<uint8_t> buf ) {
	c_packet incoming ( buf );
	const auto type = incoming.read<packet_type_t> ( );
	switch ( type ) {
		case get_computer_name_req: {
			c_packet outbound ( get_computer_name_rep );
			outbound.write<int> ( 100 );
			outbound.write<int> ( 50 );
			outbound.write<int> ( 20 );
			outbound.write<float> ( 123.f );
			outbound.write_string ( "i need a job" );
			outbound.write<float> ( 345.f );
			outbound.write_string ( std::string("hello there") );

			client.send ( outbound );

			outbound.reset ( wlatt );
			outbound.write<float> ( 69.f );

			client.send ( outbound );

			std::cout << "request handled." << std::endl;
			return false; // drop the client
		}
		default: {
			for ( const auto c : buf )
				std::cout << c;
			break;
		}
	}
	return true;
}

int main ( ) // client input thread.
{
	SetConsoleTitleA ( "tatouille server" );
	c_server server ( ip_address_t ( "10.80.59.12" ), 1605 );
	server.request_callback = call_back;
	server.serve_clients ( );
}