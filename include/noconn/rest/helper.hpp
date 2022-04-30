/*
 *
 */

#pragma once

#include <string>
#include <boost/serialization/strong_typedef.hpp>
#include <boost/asio.hpp>

namespace noconn
{
namespace rest
{
	BOOST_STRONG_TYPEDEF(unsigned short, ip_port);

	extern std::string to_string(const boost::asio::ip::address& address);
	extern std::string to_string(const boost::asio::ip::tcp::endpoint& endpoint);
	extern std::string to_string(const boost::asio::ip::tcp::socket& socket);
} // !namespace rest
} // !namespace noconn