#pragma once
#include "ip_address.h"
#include <ctime>
#include <deque>
#include <map>

class c_anti_dos {
private:
	std::map< uint32_t, std::pair<std::deque<std::clock_t>, bool> > attempts;
public:
	bool is_request_valid ( const ip_address_t &user_ip_class ) {
		const auto& user_ip = user_ip_class.binary_address;
		if ( !attempts.count ( user_ip ) ) {
			attempts [ user_ip ].first = std::deque<std::clock_t> ( );
			attempts [ user_ip ].second = false; //banned
		}

		auto& user_attempts = attempts [ user_ip ].first;
		auto& is_banned = attempts [ user_ip ].second;
		auto cur_time = std::clock ( );

		if ( is_banned ) {
			if( (cur_time - user_attempts.back()) / CLOCKS_PER_SEC <= 120 ) // unban after a while
				return false;
			else 
				is_banned = false;
		}

		if ( !user_attempts.empty ( ) ) {
			auto iter = user_attempts.begin ( ); // erase old stuff from start of arr
			while ( iter != user_attempts.end ( ) && ( cur_time - *iter ) / CLOCKS_PER_SEC > 60 )
				iter = user_attempts.erase ( iter );
		}

		is_banned = ((user_attempts.size ( ) + 1) >= 5);
		user_attempts.emplace_back ( std::clock() );
		return !is_banned;
	}
};