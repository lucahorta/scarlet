#pragma once
#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <ctime>
#include <mutex>

using lg = std::lock_guard<std::mutex>;

class c_logger {
private:
	std::mutex mtx;
	std::ostringstream stream;
	std::ofstream file_stream;
	std::string pipe_name;
	HANDLE logger_pipe;

	bool spawn_console ( );
	bool send_text ( std::string to_send );
public:
	c_logger ( );
	void clear ( );

	// this overload receive the single values to append via <<
	template<typename T>
	c_logger& operator<<( T&& value ) {
		lg lock ( mtx );
		if ( stream.str ( ) == "" ) {
			auto t = std::time ( nullptr );
			auto tm = *std::localtime ( &t );

			stream << std::put_time ( &tm, "[%d-%m-%Y %H-%M-%S] " );
		}
		stream << value;
		return *this;
	}

	// this overload intercept std::endl, to flush the stream and send all to std::cout
	c_logger& operator<<( std::ostream& ( *os )( std::ostream& ) ) {
		lg lock ( mtx );
		stream << '\n';
		send_text ( stream.str ( ) );
		stream.str ( std::string ( ) );
		return *this;
	}
};
