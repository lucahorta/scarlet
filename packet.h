#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <iostream>

enum packet_type_t : uint8_t {
	get_computer_name_req = 2,
	get_computer_name_rep,
	wlatt


};

class c_packet {
private:
	std::vector<uint8_t> data;
	uint32_t position = 0;
public:
	c_packet ( packet_type_t type ) {
		write ( ( uint8_t ) type );
	}

	c_packet ( std::vector<uint8_t>& d ) {
		data = d;
	}

	void reset ( std::vector<uint8_t>& d ) {
		data = d;
		position = 0;
	}

	void reset ( ) {
		memset ( data.data ( ), 0, data.size ( ) );
		position = 0;
	}

	void reset ( packet_type_t type ) {
		memset ( data.data ( ), 0, data.size ( ) );
		position = 0;
		write ( (uint8_t) type );
	}

	void write_string ( const std::string& s ) {
		write ( ( uint8_t ) s.size ( ) );
		for ( const auto& c : s )
			write ( ( char ) c );
	}

	template <typename T>
	void write_string ( T string_to_write ) {
		// assuming null terminated string pointer got passed in.
		auto s = reinterpret_cast<const char*>(string_to_write);
		write ((uint8_t) strlen ( s ) );
		for ( ; *s != 0; s++ )
			write ( *s );
	}

	void write ( void* to_write, uint32_t index_size ) {
		if ( data.size ( ) < position + (uint64_t)index_size )
			data.resize ( position + (uint64_t)index_size );
		memcpy ( &data [ position ], to_write, index_size );
		position += index_size;
	}

	template <typename T>
	void write ( T to_write ) {
		if ( std::is_same<T, std::string>::value ) {
			std::cout << "[!] please use the write_string function!!!";
			return;
		}
		write ( &to_write, sizeof ( T ) );
	}

	void read ( void* result, uint32_t index_size ) {
		memcpy ( result, &data [ position ], index_size );
		position += index_size;
	}

	//template <typename T>
	//std::unique_ptr<T> read ( ) {
	//	if ( std::is_same<T, std::string>::value ) {
	//		log << "[!] please use the read_string function!!!";
	//		return std::unique_ptr<T>{};
	//	}
	//	std::unique_ptr<T> p = std::make_unique<T> ( );
	//	read ( ( void* ) p.get(), sizeof ( T ) );
	//	return p;
	//}

	template <typename T>
	T read ( ) {
		if ( std::is_same<T, std::string>::value ) {
			std::cout << "[!] please use the read_string function!!!";
			return T {};
		}
		T ret;
		read ( ( void* ) &ret, sizeof ( T ) );
		return ret;
	}

	std::string read_string ( ) { // fuck char*
		auto index_size = read<uint8_t> ( );
		auto ret = std::string ( );

		for ( int i = 0; i < index_size; i++ )
			ret.push_back ( read<char> ( ) );
		return ret;
	}

	uint32_t get_size ( ) {
		return data.size ( );
	}

	const char* get_data ( ) {
		return ( const char* ) data.data ( );
	}

	std::vector<uint8_t>& get_vec ( ) {
		return data;
	}
};