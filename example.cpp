#include "server.h"
#include "packet.h"
#include <windows.h>
#include <string>

bool call_back ( const c_client client, std::vector<uint8_t> buf ) {
	c_packet incoming ( buf );
	const auto type = incoming.read<packet_type_t> ( );
	switch ( type ) {
		case get_computer_name_req: { // received packet from client.
			c_packet outbound ( get_computer_name_rep ); // creating packet of type "get_computer_name_response"
			outbound.write<int> ( 100 );
			outbound.write<int> ( 50 );
			outbound.write<int> ( 20 );
			outbound.write<float> ( 123.f ); // supports floats
			outbound.write_string ( "i need a job" );// supports c strings
			outbound.write<float> ( 345.f );
			outbound.write_string ( std::string("hello there") ); // supports std::string
			client.send ( outbound ); // sent packet to client

			outbound.reset ( wlatt ); // send another packet with different packet type
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
	SetConsoleTitleA ( "server" );
	c_server server ( ip_address_t ( "10.80.59.12" ), 1605 );
	server.request_callback = call_back;
	server.serve_clients ( );
}
