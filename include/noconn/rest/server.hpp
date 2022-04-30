/*
 *
 */

#pragma once

#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include "noconn/rest/helper.hpp"
#include "noconn/rest/connection.hpp"


namespace noconn
{
namespace rest
{
	class server;
	using shared_server = std::shared_ptr<server>;

	class server : public std::enable_shared_from_this<server>
	{
	public:
		static shared_server create(std::shared_ptr<boost::asio::io_context> io_context);

		bool open(boost::asio::ip::address address, ip_port port);
		void close();
		void close(shared_connection connection);

	protected:
		server(std::shared_ptr<boost::asio::io_context> io_context);
	
	protected:
		void do_accept();
		void handle_accept(boost::beast::error_code error_code, boost::asio::ip::tcp::socket socket);

		void handle_exit_signal(const boost::system::error_code& error, int signal);
		
	private:
		std::shared_ptr<boost::asio::io_context> m_io_context;
		boost::asio::signal_set m_signals;
		std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
		std::vector<shared_connection> m_connections;
		std::mutex m_mutex;
	};
} // !namespace rest
} // !namespace noconn