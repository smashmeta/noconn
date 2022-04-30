/*
 *
 */

#include <fmt/format.h>
#include <whatlog/logger.hpp>
#include "noconn/rest/server.hpp"

namespace noconn
{
namespace rest
{
	std::string to_string(const boost::asio::ip::address& address)
	{
		return fmt::format("{{ip: {}}}", address.to_string());
	}

	std::string to_string(const boost::asio::ip::tcp::endpoint& endpoint)
	{
		return fmt::format("{{ip: {}, port: {}}}", endpoint.address().to_string(), endpoint.port());
	}

	std::string to_string(const boost::asio::ip::tcp::socket& socket)
	{
		std::string remote_endpoint = socket.is_open() ? to_string(socket.remote_endpoint()) : "[closed]";
		return fmt::format("{{local: {} <=> remote: {}}}", to_string(socket.local_endpoint()), remote_endpoint);
	}
} // !namespace rest
} // !namespace noconn