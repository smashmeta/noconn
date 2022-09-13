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
		void send_message();
	private:
		void handle_message();
	private:
		shared_connection m_request_handler;
	};
} // !namespace rest
} // !namespace noconn