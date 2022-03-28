#include "logger.h"

bool c_logger::send_text ( std::string to_send ) {
	uint32_t chars_written;
	return WriteFile ( logger_pipe, to_send.c_str ( ), to_send.size ( ), ( DWORD* ) &chars_written, NULL ) && chars_written == to_send.size ( );
}

void c_logger::clear ( ) {
	lg lock ( mtx );
	stream.str ( "" );
	std::string s = ""; s += 0xF7;
	send_text ( s );
}

c_logger::c_logger ( ) {
	auto does_have_logger = spawn_console ( );
	if ( !does_have_logger ) {
		DisconnectNamedPipe ( logger_pipe );
		CloseHandle ( logger_pipe );

		std::remove ( "tatouille log.rat" );
		logger_pipe = CreateFile ( "tatouille log.rat",
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
	}
}

bool c_logger::spawn_console ( ) {
	pipe_name = "\\\\.\\pipe\\log";
	logger_pipe = CreateNamedPipeA ( pipe_name.c_str ( ), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 4096, 0, 1, NULL );

	if ( logger_pipe == INVALID_HANDLE_VALUE ) {
		return false;
	}

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	GetStartupInfoA ( &si );

	std::string path = "C:\\Users\\rax\\source\\repos\\logger_stub\\x64\\Release\\logger_stub.exe";
	if ( !CreateProcessA ( NULL, ( char* ) path.c_str ( ), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi ) )
		return false;

	if ( ConnectNamedPipe ( logger_pipe, NULL ) != 0 ) // weird asf...
		return true;
	return true;
}
