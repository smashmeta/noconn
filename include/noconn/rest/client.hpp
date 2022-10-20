/* 
 *
 */

#pragma once

#include "noconn/rest/connection.hpp"

namespace noconn
{
namespace rest
{
	class client
	{
	public:
		void send_response();
	private:
		void handle_request(const std::string& path, const std::string& method, const std::string& payload)
		{
			
		}
	private:
		shared_connection m_request_handler;
	};
} // !namespace rest
} // !namespace noconn